#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <vector>

struct VtfParticleSheetSequenceCandidate {
    uint32_t frames = 0;
    float timeUnits = 0.f;
};
using VtfParticleSheetCandidate = std::array<VtfParticleSheetSequenceCandidate, 64>;

inline std::optional<VtfParticleSheetCandidate> readVtfParticleSheetCandidate(
        const std::vector<uint8_t>& bytes) {
    const auto require = [](bool valid) {
        if (!valid) throw std::runtime_error("invalid VTF particle sheet");
    };
    const auto span = [&](size_t offset, size_t size) {
        return offset <= bytes.size() && size <= bytes.size() - offset;
    };
    const auto word = [&](size_t offset) {
        require(span(offset, 4));
        uint32_t value;
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    };
    const auto number = [&](size_t offset) {
        const auto bits = word(offset);
        float value;
        std::memcpy(&value, &bits, sizeof(value));
        require(std::isfinite(value));
        return value;
    };
    require(span(0, 16) && word(0) == 0x00465456 && word(4) == 7);
    const auto minor = word(8);
    if (minor == 1 || minor == 2) return {};
    require(minor >= 3 && minor <= 5 && span(0, 80));
    const size_t header = word(12), resources = word(68);
    require(resources <= 32 && header >= 80 + resources * 8 && span(0, header));
    std::optional<size_t> offset;
    for (size_t i = 0; i < resources; ++i) {
        const auto tag = word(80 + 8 * i);
        if ((tag & 0xffffffu) != 0x10u) continue;
        require(!offset && tag == 0x10u);
        offset = word(84 + 8 * i);
    }
    if (!offset) return {};
    require(*offset >= header && span(*offset, 4));
    const size_t payload = *offset + 4, size = word(*offset);
    require(size >= 8 && span(payload, size));
    const size_t end = payload + size;
    const auto version = word(payload), sequences = word(payload + 4);
    require(version <= 1 && sequences <= 64);
    const size_t frameBytes = version == 0 ? 20 : 68;

    VtfParticleSheetCandidate sheet{};
    size_t cursor = payload + 8;
    for (uint32_t index = 0; index < sequences; ++index) {
        require(end - cursor >= 16);
        const auto slot = word(cursor), frames = word(cursor + 8);
        require(slot < sheet.size() && frames <= 1024 && number(cursor + 12) >= 0.f);
        cursor += 16;
        require(frames <= (end - cursor) / frameBytes);
        float timeUnits = 0.f;
        // The lifetime initializer divides the sum of these weights by FPS.
        // The authored sequence duration and image count serve other purposes.
        for (uint32_t frame = 0; frame < frames; ++frame, cursor += frameBytes) {
            const float time = number(cursor);
            require(time >= 0.f);
            timeUnits += time;
        }
        require(std::isfinite(timeUnits));
        sheet[slot] = {frames, timeUnits};
    }
    require(cursor == end);
    return sheet;
}

inline float sourceParticleSheetLifetimeCandidate(
        float previous, int32_t sequence, float fps, const VtfParticleSheetCandidate* sheet) {
    if (!sheet || fps == 0.f) return previous;
    if (sequence < 0 || static_cast<size_t>(sequence) >= sheet->size())
        throw std::runtime_error("particle sheet sequence outside range");
    const float time = (*sheet)[sequence].timeUnits;
    return time == 0.f ? 1.f : time / fps;
}
