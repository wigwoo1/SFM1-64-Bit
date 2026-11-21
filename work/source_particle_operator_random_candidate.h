#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// The particle manager seeds its SIMD operator streams separately from a
// collection's authored scalar seed. A caller owns the stream and replay order.
inline constexpr uint32_t sourceParticleOperatorStartupSeedCandidate = 12345678u;

class SourceParticleOperatorRandomCandidate {
    std::array<std::array<float, 4>, 55> samples_{};
    size_t first_ = 23, second_ = 54;
    uint64_t drawn_ = 0;

public:
    explicit SourceParticleOperatorRandomCandidate(uint32_t seed, uint32_t streamIndex = 0) {
        uint32_t state = seed + streamIndex;
        for (auto& sample : samples_)
            for (float& lane : sample) {
                lane = static_cast<float>(state >> 16) * (1.f / 65536.f);
                state = (state + 1u) * 0xbb40e62du;
            }
    }

    uint64_t draws() const { return drawn_; }

    std::array<float, 4> next() {
        auto& output = samples_[second_];
        const auto& input = samples_[first_];
        for (size_t lane = 0; lane < output.size(); ++lane) {
            const float sum = input[lane] + output[lane];
            output[lane] = sum >= 1.f ? sum - 1.f : sum;
        }
        first_ = first_ == 0 ? samples_.size() - 1 : first_ - 1;
        second_ = second_ == 0 ? samples_.size() - 1 : second_ - 1;
        ++drawn_;
        return output;
    }
};
