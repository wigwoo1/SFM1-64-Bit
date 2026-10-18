#pragma once
#include <cstdint>
struct RegistrationFixture{uint8_t state;uint32_t flags,enabled,present;bool blocked;};
inline RegistrationFixture registrationFixture(unsigned seed){uint32_t r=(seed+1)*1664525u+1013904223u;return{uint8_t(seed),0x87654320u|((r>>24)&15),uint32_t((seed/256)%2),(r>>16)&63,(seed&512)!=0};}
inline void registrationTrace(uint32_t& t,uint32_t v){t=(t^v)*16777619u;}
