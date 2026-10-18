#pragma once
#include <cstdint>
#include <intrin.h>
// Maps CPUID SSE/SSE2 capability bits to the installed initializer's byte flags.
// This is an x64 host query, not a port of Valve's complete CPUInformation ABI.
inline std::uint8_t studioNativeMathCpuFlagsCandidate(){
 int cpu[4]{};__cpuid(cpu,0);if(cpu[0]<1)return 0;
 __cpuid(cpu,1);const auto edx=static_cast<std::uint32_t>(cpu[3]);
 return static_cast<std::uint8_t>(((edx&(1u<<25))?8:0)|((edx&(1u<<26))?16:0));
}
