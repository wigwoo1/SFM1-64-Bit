#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

inline std::vector<std::array<float, 4>> viewportCopyFixtureCandidate() {
    std::vector<float> values;
    for (unsigned index = 0; index <= 255; ++index)
        values.push_back(static_cast<float>(index) / 255.0f);
    for (float value : {-0.5f, -0.0f, 0.0001f, 0.003f, 0.0031308f, 0.0032f, 0.18f, 0.5f, 1.5f, 4.0f})
        values.push_back(value);
    std::vector<std::array<float, 4>> pixels(values.size());
    for (size_t index = 0; index < values.size(); ++index)
        pixels[index] = {values[index], values[(index + 47) % values.size()],
                         values[(index + 137) % values.size()], 1.0f};
    return pixels;
}

inline int viewportCopySrgbByteCandidate(float source) {
    const double linear = std::clamp(static_cast<double>(source), 0.0, 1.0);
    const double encoded = linear <= 0.0031308 ? 12.92 * linear
                                            : 1.055 * std::pow(linear, 1.0 / 2.4) - 0.055;
    return static_cast<int>(std::lround(255.0 * encoded));
}

inline void reportViewportCopyPixelsCandidate(const char* backend, unsigned format,
        const std::vector<std::array<uint8_t, 3>>& first,
        const std::vector<std::array<uint8_t, 3>>& second) {
    for (size_t index = 0; index < first.size(); ++index) {
        std::cout << "pixel " << backend << ' ' << format << ' ' << index;
        for (auto value : first[index]) std::cout << ' ' << unsigned(value);
        for (auto value : second.at(index)) std::cout << ' ' << unsigned(value);
        std::cout << '\n';
    }
}
