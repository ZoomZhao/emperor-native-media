// SPDX-License-Identifier: MIT
// Authored v6 rule scenarios. Drafted by Qoder DeepSeek-Flash session 80bb97b0-27bd-4d97-ad9c-5acae236701b from a written spec; reviewed and integrated by the main implementation.
#include "native_session.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

int g_checks = 0;

void need(bool condition, const char* message) {
    ++g_checks;
    if (!condition) {
        throw std::runtime_error(message);
    }
}

game_lab::NativeSessionSeed makeSeed() {
    game_lab::NativeSessionSeed seed{};
    seed.source = "test.sav";
    seed.sourceRevision = 1;
    seed.modelRevision = 2;
    seed.side = 16;
    seed.roadCost = 1;
    seed.houseCost = 10;
    seed.houseCapacity = 5;
    seed.wellCost = 10;
    seed.hunterCost = 10;
    seed.initialFunds = 10000;
    seed.buildable.assign(16u * 16u, static_cast<uint8_t>(1));
    seed.sourceRoad.assign(16u * 16u, static_cast<uint8_t>(0));
    seed.laborPercent = 42;
    seed.hunterWorkers = 15;
    seed.wellWorkers = 4;
    seed.laborPriority.fill(3);
    return seed;
}

void act(const game_lab::NativeSessionSeed& seed,
         game_lab::NativeSessionState& state,
         game_lab::NativeSessionAction action,
         int64_t x = 0,
         int64_t y = 0) {
    game_lab::NativeSessionCommand command{};
    command.action = action;
    command.expectedRevision = state.revision;
    command.x = x;
    command.y = y;
    const game_lab::NativeSessionResult result =
        game_lab::transactNativeSession(seed, state, command);
    need(result.committed,
         result.error.empty() ? "command not committed" : result.error.c_str());
}

} // namespace

int main() {
    using namespace game_lab;
    try {
        const NativeSessionSeed seed = makeSeed();

        const NativeSessionState empty = startNativeSession(seed, 6);
        need(validateNativeSession(seed, empty).empty(), "empty v6 state must validate");
        need(empty.rulesVersion == 6, "started state must report rules version 6");

        const std::string bytes = encodeNativeSession(seed, empty);
        std::string decodeError;
        std::optional<NativeSessionState> decoded =
            decodeNativeSession(seed, bytes, decodeError);
        need(decoded.has_value(), "v6 state bytes must decode with matching labor seed");
        need(decoded->rulesVersion == 6, "decoded state must report rules version 6");
        need(encodeNativeSession(seed, *decoded) == bytes,
             "v6 encode/decode must be byte identical");

        NativeSessionSeed wrongLaborPercent = makeSeed();
        wrongLaborPercent.laborPercent = 41;
        decodeError.clear();
        std::optional<NativeSessionState> badPercent =
            decodeNativeSession(wrongLaborPercent, bytes, decodeError);
        need(!badPercent.has_value(), "decode must fail when laborPercent differs");
        need(decodeError == "labor_model_mismatch",
             "laborPercent mismatch must report labor_model_mismatch");

        NativeSessionSeed wrongHunters = makeSeed();
        wrongHunters.hunterWorkers = 14;
        decodeError.clear();
        std::optional<NativeSessionState> badHunters =
            decodeNativeSession(wrongHunters, bytes, decodeError);
        need(!badHunters.has_value(), "decode must fail when hunterWorkers differs");
        need(decodeError == "labor_model_mismatch",
             "hunterWorkers mismatch must report labor_model_mismatch");

        NativeSessionSeed noLabor = makeSeed();
        noLabor.laborPercent = 0;
        const NativeSessionState noLaborV6 = startNativeSession(noLabor, 6);
        need(validateNativeSession(noLabor, noLaborV6) == "missing_labor_model",
             "v6 with zero laborPercent must report missing_labor_model");
        const NativeSessionState noLaborV5 = startNativeSession(noLabor, 5);
        need(validateNativeSession(noLabor, noLaborV5).empty(),
             "v5 must not require the v6 labor model");

        const NativeSessionState v5State = startNativeSession(seed, 5);
        const NativeLaborPlan v5Plan = planNativeLabor(seed, v5State);
        need(v5Plan.error == "labor_requires_native_v6",
             "planNativeLabor on v5 must report labor_requires_native_v6");

        NativeSessionState state = startNativeSession(seed, 6);
        for (int64_t x = 0; x < 16; ++x) {
            act(seed, state, NativeSessionAction::Road, x, 4);
        }
        act(seed, state, NativeSessionAction::House, 2, 2);
        act(seed, state, NativeSessionAction::Well, 6, 2);
        act(seed, state, NativeSessionAction::Hunter, 10, 2);
        need(nativePopulation(state) == 0,
             "newly built session must have zero population");

        const NativeLaborPlan labor = planNativeLabor(seed, state);
        need(labor.error.empty(), "v6 labor plan must succeed");
        need(labor.available == 0,
             "labor available must equal floor(population * 42 / 100)");
        need(labor.demand == 19, "labor demand must be hunterWorkers + wellWorkers");
        need(labor.employed == 0, "labor employed must be zero without population");
        need(labor.shortage == 19,
             "labor shortage must equal demand when nobody is available");
        need(labor.hunterUnderstaffed(), "hunter must be understaffed without workers");

        act(seed, state, NativeSessionAction::Resume);
        act(seed, state, NativeSessionAction::Tick);
        act(seed, state, NativeSessionAction::Tick);
        need(nativePopulation(state) == 1,
             "first v6 meal must add a resident through the understaffed-hunter path");
        need(state.foodStock == 0, "v6 arrivals must not consume meat");

        int guard = 0;
        while (nativePopulation(state) < 5 && guard < 200) {
            act(seed, state, NativeSessionAction::Tick);
            ++guard;
            need(nativePopulation(state) <= 5,
                 "population must never exceed houseCapacity");
            need(validateNativeSession(seed, state).empty(),
                 "state must remain valid during population growth");
        }
        need(nativePopulation(state) == 5,
             "population must reach the house capacity of 5");
        need(guard < 200, "population growth must converge within 200 ticks");

        const std::string advancedBytes = encodeNativeSession(seed, state);
        decodeError.clear();
        std::optional<NativeSessionState> advancedDecoded =
            decodeNativeSession(seed, advancedBytes, decodeError);
        need(advancedDecoded.has_value(), "advanced v6 state must decode");
        need(encodeNativeSession(seed, *advancedDecoded) == advancedBytes,
             "advanced v6 state encode/decode must be byte identical");

        std::cout << "{\"v6_rules_checks\":" << g_checks
                  << ",\"passed\":true}" << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}
