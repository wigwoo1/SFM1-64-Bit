#include "engine_projection_planes_candidate.h"
#include <cstdio>
#include <cstdlib>
void check(bool value){if(!value)std::abort();}
int main(){
 uint32_t planes[24];uint32_t random=123456789;
 const uint32_t special[]={0,0x80000000,0x7f800000,0xff800000,0x7fc12345,0xffc12345,1,0x80000001,0x3f800000,0xbf800000};
 for(unsigned n=0;n<65536;++n){
  for(unsigned i=0;i<24;++i){random=random*1664525u+1013904223u;planes[i]=n<10?special[(n+i)%10]:random;}
  EngineProjectionPlanesCandidate packed;engineProjectionPackPlanesCandidate(planes,packed);
  for(unsigned b=0;b<2;++b){const auto& block=b?packed.last:packed.first;
   for(unsigned c=0;c<4;++c)for(unsigned r=0;r<4;++r){
    uint32_t bits=(b&&r>=2)?0:planes[(b*4+r)*4+c];check(block.components[c][r]==bits);
    if(c<3){
     // Independent IEEE bit predicate: negatives excluding signed zero and NaN.
     bool negative=(bits>>31)&&((bits&0x7fffffff)!=0)&&((bits&0x7fffffff)<=0x7f800000);
     check(block.negative[c][r]==(negative?0xffffffff:0));check(block.absolute[c][r]==(bits&0x7fffffff));
    }
   }
  }
 }
 std::puts("PASS: 65536 six-plane patterns; exact transposition, second-block zero padding, ordered negative masks and absolute bits including signed zero/infinities/NaNs. Native execution parity and renderer integration pending.");
}
