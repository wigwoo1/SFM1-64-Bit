#pragma once
#include <cstdint>

// Bounded HDR-only scale contract. Caller supplies the selected HDR type, not
// mat_hdr_level or an assumed DX level. This is independent of exposure and
// texture-unit normalization. LDR/unknown types refuse without changing output.
// Not connected to a renderer until packed-texture sampling is verified.
inline bool sourceHdrLightmapScaleCandidate(float& output, std::int32_t selectedHdrType) {
    switch (selectedHdrType) {
    case 1: output = 16.f; return true;
    case 2: output = 1.f; return true;
    default: return false;
    }
}
