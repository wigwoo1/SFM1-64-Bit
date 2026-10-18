#include "studio_color_lookup_candidate.h"
#include "studio_color_table_candidate.h"
#include <cstdio>
#include <limits>
int main(){std::array<float,256> table;for(unsigned i=0;i<256;++i)table[i]=float(i)+10;
 unsigned cases=0;
 using R=StudioColorLookupRoundingCandidate;
 for(R mode:{R::NearestEven,R::Down,R::Up,R::TowardZero}){
  for(float input:{-1.f,-0.f,0.f,0.001f,0.002f,0.95f,1.f,std::numeric_limits<float>::infinity()}){
   const float expected=input<0?0.f:input>=0.95f?1.f:input==0?10.f:input==0.001f?(mode==R::Up?11.f:10.f):(mode==R::Down||mode==R::TowardZero?10.f:11.f);
   if(studioColorLookupCandidate(input,table,mode)!=expected)return 1;++cases;
  }
 }
 bool rejected=false;try{studioColorLookupCandidate(std::numeric_limits<float>::quiet_NaN(),table,R::NearestEven);}catch(const std::invalid_argument&){rejected=true;}if(!rejected)return 2;
 unsigned grid=0;uint64_t digest=1469598103934665603ull;
 for(R mode:{R::NearestEven,R::Down,R::Up,R::TowardZero})for(int i=-512;i<4608;++i){
  const float input=float(i)/4096.f;const float result=studioColorLookupCandidate(input,table,mode);
  uint32_t bits=0;std::memcpy(&bits,&result,4);digest=(digest^bits)*1099511628211ull;++grid;
 }
 const auto generated=studioColorTableCandidate();uint64_t actualDigest=1469598103934665603ull;unsigned actualCases=0;
 for(R mode:{R::NearestEven,R::Down,R::Up,R::TowardZero})for(int i=-512;i<4608;++i){
  const float result=studioColorLookupCandidate(float(i)/4096.f,generated,mode);
  uint32_t bits;std::memcpy(&bits,&result,4);actualDigest=(actualDigest^bits)*1099511628211ull;++actualCases;
 }
 std::printf("color_lookup_cases=%u grid_cases=%u digest=%llu generated_table_cases=%u generated_table_digest=%llu explicit_rounding_modes=4\n",cases,grid,static_cast<unsigned long long>(digest),actualCases,static_cast<unsigned long long>(actualDigest));return 0;
}

