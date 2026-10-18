#pragma once
#include <array>
#include <cstdint>
struct ArrayRangeFixture {std::array<uint32_t,5> old{},incoming{};int start=0,count=0;bool allow=false,reject=false;};
inline ArrayRangeFixture arrayRangeFixture(unsigned seed){ArrayRangeFixture f;f.start=int(seed%7)-1;f.count=int(seed/7%6);f.allow=(seed/42%2)!=0;f.reject=(seed/84%2)!=0;uint32_t r=seed/168;for(unsigned i=0;i<5;++i){r=r*1664525u+1013904223u;f.old[i]=r%6==5?0xffffffffu:r%6;r=r*1664525u+1013904223u;f.incoming[i]=r%6==5?0xffffffffu:r%6;}if(seed/168%4==0&&f.start>=0&&f.start+f.count<=5)for(int i=0;i<f.count;++i)f.incoming[i]=f.old[f.start+i];return f;}
inline uint32_t arrayRangeHash(const uint32_t*values){uint32_t h=2166136261u;for(unsigned i=0;i<5;++i)h=(h^values[i])*16777619u;return h;}
