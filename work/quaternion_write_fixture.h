#pragma once
#include <array>
#include <cstdint>
struct QuaternionWriteFixture {std::array<uint32_t,4>old{},incoming{};bool allow=false,notifyFlag=false,ownerEnabled=false;};
inline QuaternionWriteFixture quaternionWriteFixture(unsigned seed){QuaternionWriteFixture f;f.allow=(seed&2)!=0;f.notifyFlag=(seed&4)!=0;f.ownerEnabled=(seed&8)!=0;constexpr uint32_t palette[]={0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc01234,0x7f801234,0x00000001,0x3e800000};uint32_t state=seed;for(unsigned i=0;i<4;++i){state=state*1664525+1013904223;f.old[i]=palette[state%10];state=state*1664525+1013904223;f.incoming[i]=palette[state%10];}if(seed%4==0)f.incoming=f.old;if(seed%16==1){f.old={0,0x80000000,0,0};f.incoming={0x80000000,0,0x80000000,0};}return f;}
inline uint32_t quaternionWriteHash(const uint32_t*words){uint32_t hash=2166136261u;for(unsigned i=0;i<4;++i)hash=(hash^words[i])*16777619;return hash;}
