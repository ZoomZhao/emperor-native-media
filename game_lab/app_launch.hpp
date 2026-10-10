#pragma once
#include "resource_root.hpp"
#include <functional>
#include <utility>
#include <vector>

namespace game_lab {
enum class LaunchAction { Start, Help, ArgumentError, RootError, Cancelled };
struct LaunchPlan {
    LaunchAction action;
    ResourceRoot root;
};
// Arguments omit argv[0]. The chooser is explicit user input, never a path search.
// A supplied invalid root fails without falling back to another directory.
inline LaunchPlan prepareLaunch(const std::vector<std::string>& arguments,
                                const std::string& environmentRoot,
                                const std::function<std::string()>& chooseRoot) {
    std::string path = environmentRoot;
    bool supplied = false;
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (arguments[i] == "--help") return {LaunchAction::Help, {}};
        if (arguments[i] != "--data-root" || supplied || i + 1 >= arguments.size())
            return {LaunchAction::ArgumentError, {}};
        path = arguments[++i];
        supplied = true;
    }
    if (!supplied && path.empty()) {
        if (!chooseRoot) return {LaunchAction::RootError, {}};
        path = chooseRoot();
        if (path.empty()) return {LaunchAction::Cancelled, {}};
    }
    auto root = ResourceRoot::fromAbsolute(path);
    if (!root.configured()) return {LaunchAction::RootError, {}};
    return {LaunchAction::Start, std::move(root)};
}
} // namespace game_lab
