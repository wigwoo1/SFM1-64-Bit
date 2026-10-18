#include "studio_shader_color_conversion_candidate.h"
#include "studio_shader_color_conversion_owner_candidate.h"
#include "studio_color_lookup_candidate.h"
#include "studio_application_math_candidate.h"
#include <cstdio>
struct Ops {
 uint8_t a,b;uint32_t serviceMode;float converted,raw;const std::array<float,256>& table;
 uint8_t flag5aa(){return a;}uint8_t flag5a9(){return b;}uint32_t mode(){return serviceMode;}
 float convertedScale(){return converted;}float rawScale(){return raw;}
 float lookup(float value){return studioColorLookupCandidate(value,table,StudioColorLookupRoundingCandidate::NearestEven);}
};
int main(){static_assert(sizeof(void*)==8,"x64 required");
 studioInitializeApplicationMathCandidate();const auto& table=studioApplicationMathOwnerCandidate().gamma.fixedPower22;unsigned cases=0;uint64_t digest=1469598103934665603ull;
 for(unsigned config=0;config<4;++config)for(uint32_t mode=0;mode<3;++mode)for(float factor:{0.5f,1.f,2.f})for(uint32_t flag:{0u,1u,256u,257u})for(unsigned i=0;i<256;++i){
  Ops ops{uint8_t(config&1),uint8_t(config&2?128:0),mode,factor,factor*0.5f,table};
  const std::array<uint8_t,3> rgb{uint8_t(i),uint8_t(i*17),uint8_t(i*31)};
  const uint32_t value=studioShaderColorConversionCandidate(rgb,flag,ops);
  StudioShaderColorConversionOwnerCandidate owner{ops.a,ops.b,ops.converted,ops.raw};
  if(studioShaderColorConversionOwnerCandidate(owner,rgb,flag,ops)!=value)return 2;
  digest=(digest^value)*1099511628211ull;++cases;
 }
 std::printf("x64_color_conversion_cases=%u digest=%llu owned_startup_table=1\n",cases,static_cast<unsigned long long>(digest));
 std::puts("conversion_owner_grid=passed");
 return digest==6664673431726927383ull?0:1;
}

