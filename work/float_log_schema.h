#pragma once
#include <cstdint>
// Verified portions of 0x10507870/0x10508080; not the complete log schema.
struct FloatLogSchemaAttribute{const char*name;uint8_t type;uint16_t addedFlags;};
inline constexpr FloatLogSchemaAttribute floatLogSchemaRecovered[]={
 {"layers",15,0x900},{"curveinfo",1,0x900},{"usedefaultvalue",4,0},{"defaultvalue",3,0},{"bookmarks",21,0}
};
