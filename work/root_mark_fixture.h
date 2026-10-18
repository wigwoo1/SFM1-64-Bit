#pragma once
#include <array>
#include <cstdint>
struct RootFixture{std::array<uint16_t,4> masks;std::array<uint16_t,8> flags;std::array<std::array<uint32_t,3>,8> handles;std::array<bool,4> accepts;uint32_t mode,enabled,context;};
inline RootFixture rootFixture(unsigned seed){RootFixture f{};uint32_t r=seed+1;auto next=[&](){r=r*1664525u+1013904223u;return r;};f.mode=seed%6;f.enabled=(seed/6)%2;f.context=(seed/12)%2;for(unsigned i=0;i<4;++i){f.masks[i]=uint16_t(next()>>24);f.accepts[i]=(next()>>28)!=0;for(unsigned j=0;j<2;++j){unsigned k=i*2+j;f.flags[k]=uint16_t((j?15:1)|((next()>>20)&0x1800));for(auto&h:f.handles[k])h=(next()>>24)%6;}}return f;}
inline void rootTrace(uint32_t& trace,uint32_t value){trace=(trace^value)*16777619u;}
