#pragma once
#include <cstdint>
// Installed CMaterialVar vtable slot17 (10018450), independently executed.
// IsDefined is not IsTexture. Only low-nibble UNDEFINED(6) is rejected.
// The high nibble carries other state and must not affect this decision.
inline bool materialVarDefinedTypeCandidate(uint8_t typeByte)noexcept{
    return (typeByte&0x0f)!=6;
}
