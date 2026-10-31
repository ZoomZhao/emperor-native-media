// SPDX-License-Identifier: MIT
// Fidelity check for the normal App Play path. Finds two original saves of the
// same city where the later one can only be the earlier one after a forward run,
// derives the update distance from the saved scheduler, runs AppCityPlayback
// (experiment off, the same load hypotheses as play) for that many updates, and
// compares the aggregates the public playback view exposes.
//
// A pair is scored only when the mapper grids are identical, no terrain cell and
// no live building of the earlier save is missing from the later one, and the
// saved clocks are exactly one forward run apart. Anything else is reported and
// not scored: a mismatch there could be the player's edit rather than the
// simulation. Exit 0 with SKIP when EMPEROR_GAMEDATA_ROOT is unset.
#include "city_playback.hpp"
#include "../original_building_update.hpp"
#include "../original_evidence_fields.hpp"
#include "../asset_bytes.hpp"
#include <iostream>
#include <map>
#include <set>

namespace {
struct SaveView {
    std::string name;
    game_lab::OriginalSaveResult save;
    std::vector<uint8_t> payload;
    game_lab::EvidenceClock clock;
    bool clockOk = false;
    int32_t population = 0, treasury = 0, year = 0;
    bool cityOk = false;
    std::map<size_t, uint16_t> liveSlots; // slot -> type, alive 1 or 3
    size_t terrainDiffersFrom(const SaveView& later) const {
        size_t n = 0;
        for (size_t i = 0; i < save.terrain.flags.size(); ++i)
            if (save.terrain.flags[i] != later.save.terrain.flags[i]) ++n;
        return n;
    }
    bool sameMap(const SaveView& other) const {
        const auto& a = save.layers; const auto& b = other.save.layers;
        return save.terrain.activeSide == other.save.terrain.activeSide &&
            a.mapperRandom == b.mapperRandom && a.mapperFertile == b.mapperFertile && a.mapperStone == b.mapperStone;
    }
};

// Updates separating two clocks, or empty when the clocks are not exactly one
// forward run. The ported scheduler executes one phase per update and wraps at
// 50; the phase-50 tail is the only cycle and month-part increment. The
// documented valid month part is 0..15, so a month is 16 parts. That wrap is
// the service the port refuses, so a distance that crosses it is derived, not
// observed.
std::optional<unsigned> forwardDistance(const game_lab::EvidenceClock& early, const game_lab::EvidenceClock& late) {
    if (!early.rangesValid() || !late.rangesValid() || late.cycle < early.cycle) return std::nullopt;
    const unsigned tails = late.cycle - early.cycle;
    const unsigned wrapped = early.monthPart + tails;
    if (wrapped / 16 != late.month - early.month || wrapped % 16 != late.monthPart) return std::nullopt;
    if (!tails) return late.phase >= early.phase ? std::optional<unsigned>(late.phase - early.phase) : std::nullopt;
    // First tail executes phase 50: (51 - phase) updates from here. Each later
    // tail is another 51, and the tail leaves phase 0, so the target phase
    // remains.
    const uint64_t updates = uint64_t(51 - early.phase) + uint64_t(tails - 1) * 51 + late.phase;
    if (updates > std::numeric_limits<unsigned>::max()) return std::nullopt;
    return unsigned(updates);
}

std::vector<SaveView> inventory(const game_lab::ResourceRoot& root) {
    using namespace game_lab;
    std::vector<SaveView> out;
    const auto catalog = enumerateOriginalMapSources(root);
    if (!catalog.complete) throw std::runtime_error("source catalog incomplete");
    for (const auto& source : catalog.sources) {
        if (source.kind != OriginalMapSourceKind::SaveFile) continue;
        SaveView view; view.name = source.relativePath;
        view.save = loadOriginalSave(root.resolve(source.relativePath));
        if (view.save.status != OriginalSaveStatus::LoadedSnapshot) { out.push_back(std::move(view)); continue; }
        std::vector<uint8_t> raw;
        if (!detail::readFile(root.resolve(source.relativePath), 8u * 1024u * 1024u, raw)) continue;
        view.payload = decodeOriginalMapContainer(raw).payload;
        if (auto clock = evidenceSavedClock(view.payload, view.save.terrainOffset, view.save.sizeOffset)) {
            view.clock = *clock; view.clockOk = true;
        }
        OriginalBuildingUpdateContext city(OriginalBuildingUpdateScope::AdmittedTicks, true);
        if (originalBuildingSavedCityContext(view.payload, view.save.sizeOffset, city) && city.populationTotal && city.treasury && city.elapsedYears) {
            view.cityOk = true; view.population = *city.populationTotal; view.treasury = *city.treasury; view.year = *city.elapsedYears;
        }
        if (auto inputs = originalBuildingUpdateInputs(view.payload, view.save.buildings))
            for (const auto& [slot, input] : *inputs)
                if (input.alive == 1 || input.alive == 3) view.liveSlots.emplace(slot, view.save.buildings.slotTypes[slot]);
        out.push_back(std::move(view));
    }
    return out;
}

// A slot that is live earlier and gone later was removed. A type change of a
// slot that survives can be the simulation evolving a house, so it is not
// treated as the player's action.
size_t removedBuildings(const SaveView& early, const SaveView& late) {
    size_t removed = 0;
    for (const auto& [slot, type] : early.liveSlots) if (!late.liveSlots.count(slot)) ++removed;
    return removed;
}
}

int main() {
    using namespace game_lab;
    const char* rootPath = std::getenv("EMPEROR_GAMEDATA_ROOT");
    if (!rootPath || !*rootPath) { std::cout << "SKIP EMPEROR_GAMEDATA_ROOT unset\n"; return 0; }
    const auto root = ResourceRoot::fromAbsolute(rootPath);
    if (!root.configured()) { std::cerr << "data root not configured\n"; return 1; }
    AppCityPlaybackPreferences preferences; std::string error;
    if (!readAppCityPlaybackPreferences(root, preferences, error)) { std::cerr << error << '\n'; return 1; }

    const auto saves = inventory(root);
    std::cout << "FIDELITY saves " << saves.size() << "\n";
    for (const auto& s : saves) {
        std::cout << "save " << s.name << " side=" << s.save.terrain.activeSide;
        if (s.clockOk) std::cout << " phase=" << s.clock.phase << " part=" << s.clock.monthPart
                                 << " cycle=" << s.clock.cycle << " month=" << s.clock.month;
        if (s.cityOk) std::cout << " year=" << s.year << " pop=" << s.population << " treasury=" << s.treasury;
        std::cout << " buildings=" << s.liveSlots.size()
                  << " figures=" << (s.save.layers.figures ? s.save.layers.figures->live.size() : 0) << "\n";
    }

    unsigned compared = 0, matched = 0, blocked = 0, rejected = 0;
    for (size_t i = 0; i < saves.size(); ++i) for (size_t j = 0; j < saves.size(); ++j) {
        if (i == j) continue;
        const auto& early = saves[i]; const auto& late = saves[j];
        if (!early.clockOk || !late.clockOk || !early.sameMap(late)) continue;
        const auto distance = forwardDistance(early.clock, late.clock);
        if (!distance || !*distance) continue; // not a forward run, or the same instant
        const size_t terrain = early.terrainDiffersFrom(late), removed = removedBuildings(early, late);
        std::cout << "pair " << early.name << " -> " << late.name
                  << " terrainDiff=" << terrain << " removedBuildings=" << removed
                  << " distance=" << *distance;
        if (terrain || removed) {
            ++rejected;
            std::cout << " rejected player-action:";
            if (terrain) std::cout << " terrain";
            if (removed) std::cout << " demolition";
            std::cout << "\n";
            continue;
        }
        AppMapSurface surface(root); AppCitySessionState session; session.crtRandom = 1;
        AppCityPlayback playback(surface, early.save, early.payload, preferences, 1, 0, false, &session);
        if (!playback.prepared()) { ++blocked; std::cout << " blocked not-prepared " << playback.status().error << "\n"; continue; }
        const unsigned interval = std::max(1u, playback.status().intervalMs);
        for (unsigned n = 1; n <= *distance; ++n) if (!playback.step(uint64_t(n) * interval)) break;
        const bool reached = playback.status().updates == *distance && playback.status().refusal.empty();
        std::cout << " reached=" << playback.status().updates << "/" << *distance
                  << " refusal=" << (playback.status().refusal.empty() ? "none" : playback.status().refusal);
        if (!reached) { ++blocked; std::cout << " blocked\n"; continue; }
        const auto finances = playback.tributeFinances();
        const auto calendar = playback.status().calendar;
        auto compare = [&](const std::string& field, auto got, auto want) {
            const bool same = got == want; ++compared; if (same) ++matched;
            std::cout << "  " << field << " ours=" << got << " original=" << want << (same ? " match" : " MISMATCH") << "\n";
        };
        std::cout << "\n";
        if (finances && late.cityOk) compare("treasury", finances->treasury, late.treasury);
        if (calendar) { compare("month", calendar->first, late.clock.month); compare("monthPart", calendar->second, late.clock.monthPart); }
    }
    std::cout << "FIDELITY " << matched << "/" << compared
              << " blocked=" << blocked << " rejected=" << rejected << "\n";
    return 0;
}
