#pragma once
#include <array>
#include <cstdint>
struct FloatArrayWriteFixture{std::array<uint32_t,4> old;uint32_t incoming;int32_t index;int32_t count;bool allowed;uint32_t sourceType;};
inline FloatArrayWriteFixture floatArrayWriteFixture(unsigned seed){
 const uint32_t bits[]={0,0x80000000u,0x3f800000u,0xbf800000u,0x7f800000u,0xff800000u,0x7fc12345u,0x7f812345u,1,0x807fffffu,0x7f7fffffu,0xff812345u};
 FloatArrayWriteFixture f{};
 for(unsigned i=0;i<4;++i)f.old[i]=bits[(seed+i*3)%12];
 f.index=int32_t((seed/12)%7)-2;f.count=int32_t((seed/84)%5);
 f.incoming=bits[(seed/420)%12];f.allowed=((seed/5040)%2)!=0;
 f.sourceType=((seed/10080)%2)?10:3;return f;
}
inline uint32_t floatArrayWriteHash(const uint32_t* values){uint32_t h=2166136261u;for(unsigned i=0;i<4;++i){h^=values[i];h*=16777619u;}return h;}
