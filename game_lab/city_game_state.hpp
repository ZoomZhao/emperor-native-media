#pragma once
#include "city_state_io.hpp"
#include "app/city_scenario_checkpoint.hpp"
#include "app/city_warehouse_training.hpp"
#include <sys/file.h>
#include <CommonCrypto/CommonDigest.h>

namespace game_lab {
// Self-authored single-file challenge saves. Free cities retain the complete EMP1 contract.
constexpr size_t kCityGameHeaderBytes = 20 + CC_SHA256_DIGEST_LENGTH;
constexpr size_t kCityClassicGameMaxBytes = kCityGameHeaderBytes + kCityStateBytes +
    kCityScenarioCheckpointBytes + 12 + 8 * CityScenarioAwardBook::kMaxEntries;
constexpr size_t kCityWarehouseGameMaxBytes = kCityGameHeaderBytes + kCityWarehouseStateBytes +
    kCityScenarioCheckpointBytes + 12 + 8 * CityScenarioAwardBook::kMaxEntries;
constexpr size_t kCityGameMaxBytes = std::max(kCityClassicGameMaxBytes,kCityWarehouseGameMaxBytes);

namespace city_game_io_detail {
inline bool writeAtomic(const std::vector<uint8_t>& bytes, const std::filesystem::path& path,
                        std::string& error) {
    if (path.empty()) { error = "No save path selected"; return false; }
    const auto pattern = path.string() + ".tmp.XXXXXX";
    std::vector<char> temporary(pattern.begin(), pattern.end()); temporary.push_back(0);
    const int fd = mkstemp(temporary.data());
    if (fd < 0) { error = "Cannot create game save"; return false; }
    size_t offset = 0; bool ok = true;
    while (offset < bytes.size()) {
        const auto count = ::write(fd, bytes.data() + offset, bytes.size() - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { ok = false; break; }
        offset += size_t(count);
    }
    if (ok && fsync(fd) != 0) ok = false;
    if (close(fd) != 0) ok = false;
    if (ok && rename(temporary.data(), path.c_str()) != 0) ok = false;
    if (!ok) { unlink(temporary.data()); error = "Cannot write or replace game save"; return false; }
    return true;
}
inline bool readBounded(const std::filesystem::path& path, size_t limit, std::vector<uint8_t>& bytes,
                        std::string& error) {
    if (path.empty()) { error = "No save path selected"; return false; }
    const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW);
    if (fd < 0) { error = "Cannot open game save"; return false; }
    struct stat info{};
    if (fstat(fd, &info) != 0 || !S_ISREG(info.st_mode) || info.st_size < 0 || uint64_t(info.st_size) > limit) {
        close(fd); error = "Game save must be a bounded ordinary file"; return false;
    }
    std::vector<uint8_t> candidate(size_t(info.st_size)); size_t offset = 0; bool ok = true;
    while (offset < candidate.size()) {
        const auto count = read(fd, candidate.data() + offset, candidate.size() - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { ok = false; break; }
        offset += size_t(count);
    }
    uint8_t extra = 0; ssize_t tail;
    do { tail = read(fd, &extra, 1); } while (tail < 0 && errno == EINTR);
    if (tail != 0) ok = false;
    if (close(fd) != 0) ok = false;
    if (!ok) { error = "Incomplete or changing game save"; return false; }
    bytes = std::move(candidate); return true;
}
inline bool magic(const std::vector<uint8_t>& b, const char* m) {
    return b.size() >= 4 && std::equal(b.begin(), b.begin() + 4, m);
}
inline std::array<uint8_t,CC_SHA256_DIGEST_LENGTH> checksum(const std::vector<uint8_t>& bytes) {
    std::array<uint8_t,CC_SHA256_DIGEST_LENGTH> out{};
    CC_SHA256(bytes.data(),CC_LONG(bytes.size()),out.data());
    return out;
}
} // namespace city_game_io_detail

inline std::vector<uint8_t> encodeCityGameState(const CitySimulation& city,
    const CityScenarioSession& session, const CityScenarioAwardBook& book, std::string& error) {
    error.clear();
    const auto cityBytes = city.saveSnapshot();
    if (!session.active()) return cityBytes;
    const auto& s = city.stats(); int64_t month = 0;
    const bool training = isCityWarehouseTrainingSpec(session.checkpoint().spec);
    const bool classic = city.ruleset() == CityRuleset::StaffedServices &&
        session.checkpoint().spec.id != 4;
    if ((!classic && !(training && city.ruleset() == CityRuleset::RoadWarehouses)) ||
        !city_scenario_detail::monthNumber({s.year,s.month},month) ||
        session.checkpoint().lastObservedMonth != month) {
        error = "Challenge rule, registered training revision and current city month must match"; return {};
    }
    auto metadata = encodeCityScenarioCheckpoint(session.checkpoint(), error);
    if (metadata.empty()) return {};
    const auto awards = encodeCityScenarioAwards(book,error);
    if (awards.empty()) return {};
    std::vector<uint8_t> bytes{'E','C','G','1'};
    for (uint64_t n : {uint64_t(training ? 2 : 1),uint64_t(cityBytes.size()),uint64_t(metadata.size()),uint64_t(awards.size())})
        city_scenario_codec_detail::put(bytes,n,4);
    bytes.insert(bytes.end(),cityBytes.begin(),cityBytes.end());
    bytes.insert(bytes.end(),metadata.begin(),metadata.end());
    bytes.insert(bytes.end(),awards.begin(),awards.end());
    const auto digest = city_game_io_detail::checksum(bytes);
    bytes.insert(bytes.begin()+20,digest.begin(),digest.end());
    return bytes;
}

inline bool decodeCityGameState(const std::vector<uint8_t>& bytes, CitySimulation& city,
    CityScenarioSession& session, CityScenarioAwardBook& book, std::string& error) {
    error.clear(); CitySimulation candidate; CityScenarioSession challenge; auto awards = book;
    if (!city_scenario_detail::validBook(awards)) { error = "Invalid live scenario receipts"; return false; }
    if (city_game_io_detail::magic(bytes,"EMP1")) {
        if (!candidate.loadSnapshot(bytes)) { error = "Invalid EMP1 city"; return false; }
    } else {
        if (bytes.size() < kCityGameHeaderBytes || bytes.size() > kCityGameMaxBytes ||
            !city_game_io_detail::magic(bytes,"ECG1")) { error = "Unknown self-authored save format"; return false; }
        size_t offset = 4;
        const auto get = [&]() { return city_scenario_codec_detail::get(bytes,offset,4); };
        const auto version = get(), citySize = get(), scenarioSize = get(), awardSize = get();
        const bool classic = version == 1 && citySize == kCityStateBytes;
        const bool training = version == 2 && citySize == kCityWarehouseStateBytes;
        if ((!classic && !training) || scenarioSize != kCityScenarioCheckpointBytes ||
            awardSize < 12 || awardSize > 12 + 8 * CityScenarioAwardBook::kMaxEntries ||
            bytes.size() != kCityGameHeaderBytes + citySize + scenarioSize + awardSize) {
            error = "Invalid complete challenge container lengths/version"; return false;
        }
        std::vector<uint8_t> hashedBytes(bytes.begin(),bytes.begin()+20);
        hashedBytes.insert(hashedBytes.end(),bytes.begin()+kCityGameHeaderBytes,bytes.end());
        const auto digest=city_game_io_detail::checksum(hashedBytes);
        if (!std::equal(digest.begin(),digest.end(),bytes.begin()+20)) {
            error = "City and challenge checksum mismatch; no partial restore"; return false;
        }
        offset=kCityGameHeaderBytes;
        const auto part = [&](size_t n) {
            std::vector<uint8_t> result(bytes.begin()+offset,bytes.begin()+offset+n); offset += n; return result;
        };
        if (!candidate.loadSnapshot(part(size_t(citySize))) ||
            candidate.ruleset() != (training ? CityRuleset::RoadWarehouses : CityRuleset::StaffedServices)) {
            error = "Invalid challenge city rule/version pairing"; return false;
        }
        const auto metadata = part(size_t(scenarioSize)); CityScenarioAwardBook savedAwards;
        if (!decodeCityScenarioAwards(part(size_t(awardSize)),savedAwards,error) ||
            !mergeCityScenarioAwards(awards,savedAwards,error)) return false;
        CityScenarioCheckpoint saved;
        if (!decodeCityScenarioCheckpoint(metadata,saved,error)) return false;
        if (training) {
            const auto result = restoreCityWarehouseTraining(challenge,saved,candidate,awards);
            if (!result.ok) { error = result.message; return false; }
        } else {
            if (saved.spec.id == 4) { error = "Warehouse training requires ECG1 version2 and warehouse rules"; return false; }
            const auto& s = candidate.stats();
            const auto result = challenge.restore(saved,{s.year,s.month},awards);
            if (!result.ok) { error = result.message; return false; }
        }
    }
    city = std::move(candidate); session = std::move(challenge); book = std::move(awards);
    return true;
}

inline bool saveCityGameState(const CitySimulation& city, const CityScenarioSession& session,
    const CityScenarioAwardBook& book, const std::filesystem::path& path, std::string& error) {
    const auto bytes = encodeCityGameState(city,session,book,error);
    if (bytes.empty()) return false;
    CitySimulation probe; CityScenarioSession probeSession; auto probeBook = book;
    if (!decodeCityGameState(bytes,probe,probeSession,probeBook,error)) return false;
    return city_game_io_detail::writeAtomic(bytes,path,error);
}
inline bool loadCityGameState(CitySimulation& city, CityScenarioSession& session,
    CityScenarioAwardBook& book, const std::filesystem::path& path, std::string& error) {
    error.clear(); std::vector<uint8_t> bytes;
    if (!city_game_io_detail::readBounded(path,std::max(kCityGameMaxBytes,kCityWarehouseStateBytes),bytes,error)) return false;
    return decodeCityGameState(bytes,city,session,book,error);
}

inline uint64_t assignedCityRunFloor(const CityScenarioSession& session, const CityScenarioAwardBook& book) {
    uint64_t floor = session.active() ? session.checkpoint().runId : 0;
    for (auto run : book.completedRuns) floor = std::max(floor,run);
    return floor;
}
// A private App profile counter, locked across callers. Never reset on city load or new challenge.
// Invalid/torn counters fail closed. The caller must preserve this file; no crash-recovery claim.
inline bool allocateCityRunId(const std::filesystem::path& path, uint64_t minimum, uint64_t& out,
                             std::string& error) {
    error.clear();
    if (path.empty()) { error = "No App run-counter path"; return false; }
    const int lock = open((path.string()+".lock").c_str(),O_RDWR|O_CREAT|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK,0600);
    struct stat info{};
    if (lock < 0) { error = "Cannot open App run-counter lock"; return false; }
    if (fstat(lock,&info) != 0 || !S_ISREG(info.st_mode) || flock(lock,LOCK_EX|LOCK_NB) != 0) {
        close(lock); error = "App run-counter is busy or invalid"; return false;
    }
    uint64_t last = minimum; bool ok = true;
    struct stat counterInfo{};
    if (lstat(path.c_str(),&counterInfo) == 0) {
        std::vector<uint8_t> bytes;
        ok = city_game_io_detail::readBounded(path,16,bytes,error);
        if (ok && (bytes.size()!=16 || !city_game_io_detail::magic(bytes,"ECR1"))) ok = false;
        if (ok) {
            size_t offset=4;
            if (city_scenario_codec_detail::get(bytes,offset,4)!=1) ok=false;
            else last=std::max(last,city_scenario_codec_detail::get(bytes,offset,8));
        }
    } else if (errno!=ENOENT) ok=false;
    if (ok && last==std::numeric_limits<uint64_t>::max()) ok=false;
    if (ok) {
        std::vector<uint8_t> bytes{'E','C','R','1'};
        city_scenario_codec_detail::put(bytes,1,4); city_scenario_codec_detail::put(bytes,last+1,8);
        ok=city_game_io_detail::writeAtomic(bytes,path,error);
    }
    flock(lock,LOCK_UN); close(lock);
    if (!ok) { if (error.empty()) error="Invalid/exhausted App run counter; no run started"; return false; }
    out=last+1; return true;
}
} // namespace game_lab
