#pragma once
#include <intrin.h>
#include <array>
#include <cstdint>
// Snapshot adapter only: original cache initialization/order and affinity
// sampling are not implemented by this service.
struct NativeCpuMaskServicesCandidate{
 std::array<uint32_t,4> basic=cpuid(0,0),extended=cpuid(0x80000000u,0);
 // Original10002ec0 ORs arguments into EAX and clears ECX.
 std::array<uint32_t,4> cpuid(uint32_t leaf,uint32_t prefix){int registers[4];__cpuidex(registers,static_cast<int>(leaf|prefix),0);return {static_cast<uint32_t>(registers[0]),static_cast<uint32_t>(registers[1]),static_cast<uint32_t>(registers[2]),static_cast<uint32_t>(registers[3])};}
 uint32_t maximumBasic(){return basic[0];}
 uint32_t maximumExtended(){return extended[0];}
 bool isIntel(){return basic[1]==0x756e6547u&&basic[3]==0x49656e69u&&basic[2]==0x6c65746eu;}
};
