#pragma once
#include <cstdint>
// Installed ReadHeader normalizes legacy flags before VTF state publication.
// Keep raw flags separately. This does not validate an image or change the
// existing raw VTF layout reader or production renderer.
inline bool vtfEffectiveFlagsCandidate(uint32_t major,uint32_t minor,uint32_t raw,uint32_t&effective)noexcept{
    if(major!=7||minor<1||minor>5)return false;
    effective=minor<=3?raw&0x2e87fbffu:raw;return true;
}
