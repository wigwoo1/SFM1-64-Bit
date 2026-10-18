#include "source_optimized_box_record_candidate.h"
#include <iostream>
#include <limits>
int main(){try{
 std::vector<SourceBoxRecordPlaneCandidate>planes(6);std::array<SourceBoxRecordSideCandidate,6>sides{};std::vector<uint32_t>surfaces{0x12345678};
 for(unsigned i=0;i<6;++i){planes[i].type=uint8_t(i/2);planes[i].normal[i/2]=i%2?1.0f:-1.0f;planes[i].distance=1;sides[i]={i,0,0};}
 for(unsigned test=0;test<5;++test){auto p=planes;auto s=sides;
  // Corrupt the last side so earlier work must not leak into the output.
  if(test==0)s[5].plane=99;
  if(test==1)s[5].texture=1;
  if(test==2)p[5].type=3;
  if(test==3)p[5].distance=std::numeric_limits<float>::infinity();
  if(test==4)p[5].normal[1]=std::numeric_limits<float>::quiet_NaN();
  std::array<uint8_t,48>out;out.fill(0xa5);const auto before=out;bool rejected=false;try{sourceOptimizedBoxRecordCandidate(p,s,surfaces,out);}catch(const std::runtime_error&){rejected=true;}
  if(!rejected||out!=before)throw std::runtime_error("invalid input changed output or was accepted");
 }
 std::array<uint8_t,48>out{};sourceOptimizedBoxRecordCandidate(planes,sides,surfaces,out);uint16_t surface;std::memcpy(&surface,out.data()+32,2);if(surface!=0x5678)throw std::runtime_error("surface truncation mismatch");
 std::cout<<"PASS five rejected inputs preserve output; surface truncation\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
