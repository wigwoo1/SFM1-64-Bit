#include "point_constraint_accumulator.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdint>
int main(int argc, char** argv) {
    static_assert(sizeof(void*) == 8, "64-bit replacement required");
    if (argc != 2) return 2;
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) return 2;
    const std::array<float, 8> weights{-2, -.5f, 0, .125f, .5f, 1, 2, 8};
    unsigned checks = 0, mismatches = 0;
    uint32_t count, pattern, missing;
    while (input.read(reinterpret_cast<char*>(&count), 4)) {
        std::array<float, 4> expected{};
        input.read(reinterpret_cast<char*>(&pattern), 4);
        input.read(reinterpret_cast<char*>(&missing), 4);
        input.read(reinterpret_cast<char*>(expected.data()), 16);
        if (!input || count > 4 || pattern >= 64 || missing >= 16) return 2;
        std::vector<RecoveredPointConstraintTarget> targets;
        for (unsigned i = 0; i < count; ++i) targets.push_back({
            {float(i*3)-4.125f, float(pattern)*.125f-2, float(i)*-.75f},
            {.125f*float(i), -.5f, 1.25f}, weights[(pattern+i*3)%8],
            (missing & (1u << i)) == 0});
        auto actual = accumulateRecoveredPointConstraint(targets, {-.25f, .5f, 1.5f});
        std::array<float, 4> result{actual.position[0], actual.position[1],
            actual.position[2], actual.influence};
        if (std::memcmp(result.data(), expected.data(), 16)) ++mismatches;
        ++checks;
    }
    std::cout << "point_average_checks=" << checks << " mismatches=" << mismatches << '\n';
    return checks == 5120 && mismatches == 0 ? 0 : 1;
}
