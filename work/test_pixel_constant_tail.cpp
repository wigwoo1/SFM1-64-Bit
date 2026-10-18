#include "studio_pixel_constant_tail_candidate.h"
#include <cstdio>
#include <vector>
struct Ops{
 std::array<uint32_t,120> current{},comparison{},nextCurrent{},nextComparison{};
 uint32_t* currentPtr=current.data();uint32_t* comparisonPtr=comparison.data();
 bool replace=false;unsigned submits=0;std::vector<unsigned> events;
 uint32_t* comparisonCache(){events.push_back(1);return comparisonPtr;}
 uint32_t* currentCache(){events.push_back(3);return currentPtr;}
 void submitPixel(uint32_t index,uint32_t count,const uint32_t* words){
  if(index!=29||count!=1||words[1]!=0x7fc12345)throw 1;
  events.push_back(2);++submits;
  if(replace){currentPtr=nextCurrent.data();comparisonPtr=nextComparison.data();}
 }
};
int main(){unsigned cases=0;
 for(unsigned changed=0;changed<2;++changed)for(unsigned replace=0;replace<2;++replace){
  Ops ops;ops.replace=replace!=0;
  const std::array<uint32_t,4> local{0,0x7fc12345,0x3f800000,0xff800000};
  std::memcpy(ops.comparison.data()+116,local.data(),16);
  if(changed)ops.comparison[116]=0x80000000;
  const bool result=studioPixelConstantTailCandidate(local,ops);
  if(result!=bool(changed)||ops.submits!=changed)return 2;
  if(changed){
   if(ops.events!=std::vector<unsigned>{1,2,3,1})return 3;
   if(std::memcmp(ops.currentPtr+116,local.data(),16)||std::memcmp(ops.comparisonPtr+116,local.data(),16))return 4;
   if(replace&&(ops.current[116]!=0||ops.comparison[116]!=0x80000000))return 5;
  }else if(ops.events!=std::vector<unsigned>{1})return 6;
  ++cases;
 }
 std::printf("pixel_constant_tail_cases=%u raw_nan_zero_equality=1 cache_publication_order=1 pointer_reload=1\n",cases);
}
