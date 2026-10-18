#include "studio_transition_vertex_values_candidate.h"
#include <cstdio>
struct Ops{
 bool modern,equal;float hi=6,densityValue=-1;unsigned reads=0;
 float upper(){++reads;return equal?2.f:hi;}float lower(){++reads;return 2;}
 int capability(){hi=100;densityValue=.5f;return modern?91:90;}
 float density(){++reads;return densityValue;}
};
int main(){unsigned cases=0;
 for(unsigned active=0;active<2;++active)for(unsigned modern=0;modern<2;++modern)for(unsigned equal=0;equal<2;++equal){
  Ops ops{modern!=0,equal!=0};const auto bits=studioTransitionVertexValuesCandidate(active!=0,ops);
  if(!active){const std::array<uint32_t,4> expected=modern?std::array<uint32_t,4>{0,0xff7fffff,0,0}:std::array<uint32_t,4>{0x3f800000,0xff7fffff,0x3f800000,0};if(bits!=expected||ops.reads)return 1;}
  else{
   std::array<float,4> values{};std::memcpy(values.data(),bits.data(),16);
   const float product=equal?2.f:1.5f;
   if(values!=std::array<float,4>{modern?1.f-product:product,0,.5f,equal?1.f:.25f}||ops.reads!=3)return 2;
  }
  ++cases;
 }
 std::printf("transition_vertex_value_cases=%u captured_bounds_live_density=1\n",cases);
}
