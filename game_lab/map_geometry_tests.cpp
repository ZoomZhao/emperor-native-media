#include "map_geometry.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

struct Case {
    const char* name;
    double x, y, width, height;
    int dimension;
    bool accepted;
    int expectedX, expectedY;
};

int main() {
    const double largest = std::numeric_limits<double>::max();
    const double tiniest = std::numeric_limits<double>::denorm_min();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    const int largestDimension = std::numeric_limits<int>::max();
    std::vector<Case> cases = {
        {"middle", 1, .5, 2, 1, 128, true, 64, 64},
        {"finite-largest-width", largest / 2, .5, largest, 1, 128, true, 64, 64},
        {"finite-largest-height", 1, largest / 2, 2, largest, 128, true, 64, 64},
        {"finite-largest-both", largest / 2, largest / 2, largest, largest, 128, true, 64, 64},
        {"bottom-left", 0, 0, 2, 1, 128, true, 0, 127},
        {"bottom-right-inside", std::nextafter(2.0, 0.0), 0, 2, 1, 128, true, 127, 127},
        {"top-left-inside", 0, std::nextafter(1.0, 0.0), 2, 1, 128, true, 0, 0},
        {"top-right-inside", std::nextafter(2.0, 0.0), std::nextafter(1.0, 0.0), 2, 1, 128, true, 127, 0},
        {"tiny-finite-corner", 0, 0, tiniest, tiniest, 128, true, 0, 127},
        {"one-tile", 1, .5, 2, 1, 1, true, 0, 0},
        {"largest-dimension-middle", 1, .5, 2, 1, largestDimension, true, 1073741823, 1073741823},
        {"largest-dimension-bottom", 0, 0, 2, 1, largestDimension, true, 0, largestDimension - 1},
        {"negative-x", -1, .5, 2, 1, 128, false, -77, -88},
        {"negative-y", 1, -.5, 2, 1, 128, false, -77, -88},
        {"right-edge-outside", 2, .5, 2, 1, 128, false, -77, -88},
        {"top-edge-outside", 1, 1, 2, 1, 128, false, -77, -88},
        {"zero-width", 0, .5, 0, 1, 128, false, -77, -88},
        {"zero-height", 1, 0, 2, 0, 128, false, -77, -88},
        {"negative-width", 0, .5, -2, 1, 128, false, -77, -88},
        {"negative-height", 1, 0, 2, -1, 128, false, -77, -88},
        {"zero-dimension", 1, .5, 2, 1, 0, false, -77, -88},
        {"negative-dimension", 1, .5, 2, 1, -128, false, -77, -88},
    };
    for (double bad : {nan, infinity, -infinity}) {
        cases.push_back({"nonfinite-x", bad, .5, 2, 1, 128, false, -77, -88});
        cases.push_back({"nonfinite-y", 1, bad, 2, 1, 128, false, -77, -88});
        cases.push_back({"nonfinite-width", 1, .5, bad, 1, 128, false, -77, -88});
        cases.push_back({"nonfinite-height", 1, .5, 2, bad, 128, false, -77, -88});
    }
    unsigned failures = 0;
    for (const auto& c : cases) {
        int x = -77, y = -88;
        bool accepted = game_lab::mapPointToTile(c.x, c.y, c.width, c.height, c.dimension, x, y);
        if (accepted != c.accepted || x != c.expectedX || y != c.expectedY) {
            std::cerr << c.name << ": accepted=" << accepted << " x=" << x << " y=" << y << '\n';
            ++failures;
        }
    }
    std::cout << "{\"passed\":" << (failures == 0 ? "true" : "false")
              << ",\"cases\":" << cases.size() << ",\"failures\":" << failures
              << ",\"city_model_executed\":false,\"GameData\":false,\"ui\":false,\"audio_devices\":false}\n";
    return failures == 0 ? 0 : 1;
}
