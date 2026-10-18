#include "studio_aux_pixel_constant_candidate.h"
#include <cstdio>
#include <limits>
struct Ops{
 uint32_t selected=0,published=0;unsigned reads=0;float lo=2,hi=4,value=.5f;
 std::array<uint32_t,120> current{},comparison{};unsigned uploads=0;
 uint32_t* currentCache(){return current.data();}uint32_t* comparisonCache(){return comparison.data();}
 void submitPixel(unsigned index,unsigned count,const uint32_t*){if(index!=17||count!=1)throw 2;++uploads;published=99;}
 void publishIndex(uint32_t index){published=index;}
 uint32_t mode(){if(published!=17)throw 1;hi=6;return selected;}
 float lower(){++reads;return lo;}float upper(){++reads;return hi;}
 float second(){++reads;return 3;}float density(){++reads;return value;}
};
int main(){unsigned cases=0;
 for(unsigned mode=0;mode<2;++mode)for(unsigned enable=0;enable<2;++enable)
 for(float density:{-1.f,.5f,2.f,std::numeric_limits<float>::quiet_NaN()}){
  Ops ops;ops.selected=mode;ops.value=density;
  const auto bits=studioAuxPixelConstantCandidate(17,enable,ops);
  if(!mode||!enable){if(bits!=std::array<uint32_t,4>{0,0xff7fffff,0,0}||ops.reads)return 2;}
  else{
   std::array<float,4> values{};std::memcpy(values.data(),bits.data(),16);
   if(values[0]!=-.5f||values[1]!=3||values[3]!=.25f||ops.reads!=4)return 3;
   if(density!=density){if(values[2]==values[2])return 4;}
   else if(values[2]!=(density<0?0:density>1?1:density))return 5;
  }
  ops.comparison.fill(0xcccccccc);
  if(!studioAuxPixelConstantUploadCandidate(17,enable,ops)||ops.uploads!=1)return 6;
  if(std::memcmp(ops.current.data()+68,bits.data(),16)||std::memcmp(ops.comparison.data()+68,bits.data(),16))return 7;
  if(studioAuxPixelConstantUploadCandidate(17,enable,ops)||ops.uploads!=1)return 8;
  ++cases;
 }
 std::printf("aux_pixel_constant_cases=%u index_before_callback=1 live_parameter_reads=1\n",cases);
}
