#include "studio_pixel_color_candidate.h"
#include "studio_pixel_color_owner_candidate.h"
#include <fstream>
#include <cstdio>
#include <cmath>
struct Ops{
 unsigned selected,service;bool captured,suppress;float e;bool powerEnabled;
 StudioPixelExposureCandidate exposure(){return {e,e>0};}
 unsigned mode(){return selected;}float channel(unsigned i){return float(i+1);}
 bool transformCaptured(){return captured;}bool usePower(){return powerEnabled;}
 float transform(float x){return x+1;}double power(double x,double y){return std::pow(x,y);}
 bool suppressScale(){return suppress;}unsigned serviceMode(){return service;}float scale(){return 2;}
 unsigned uploads=0;
 void submitPixel(unsigned index,unsigned count,const uint32_t*){if(index!=29||count!=1)throw 1;++uploads;}
};
int main(){
 static_assert(sizeof(void*)==8,"x64 required");
 std::ifstream oracle("outputs/native-pixel-color-grid.bin",std::ios::binary);if(!oracle)return 1;
 unsigned cases=0;uint64_t digest=1469598103934665603ull;
 for(unsigned mode=0;mode<3;++mode)for(unsigned service=0;service<2;++service)
 for(unsigned flags=0;flags<4;++flags)for(float e:{0.f,2.f,-2.f})for(unsigned power=0;power<2;++power){
  Ops ops{mode,service,bool(flags&1),bool(flags&2),e,power!=0};
  const auto result=studioPixelColorCandidate(ops);std::array<uint32_t,4> native{},bits{};
  oracle.read(reinterpret_cast<char*>(native.data()),16);if(!oracle)return 2;
  std::memcpy(bits.data(),result.data(),16);
  if(bits!=native){std::printf("pixel_color_mismatch=%u\n",cases);return 3;}
  std::array<uint32_t,120> current{},comparison{};comparison.fill(0xcccccccc);
  StudioPixelColorOwnerCandidate owner{{1,2,3},ops.captured,ops.suppress,2,current.data(),comparison.data()};
  if(!studioPixelColorOwnerUploadCandidate(owner,ops)||ops.uploads!=1)return 5;
  if(std::memcmp(current.data()+116,native.data(),16)||std::memcmp(comparison.data()+116,native.data(),16))return 6;
  if(studioPixelColorOwnerUploadCandidate(owner,ops)||ops.uploads!=1)return 7;
  for(auto word:bits)digest=(digest^word)*1099511628211ull;++cases;
 }
 if(oracle.peek()!=std::char_traits<char>::eof())return 4;
 std::printf("x64_native_pixel_color_cases=%u digest=%llu\n",cases,static_cast<unsigned long long>(digest));
 std::puts("pixel_color_owner_native_grid_cache_reuse=passed");
 unsigned mutationCases=0;
 for(unsigned selected:{1u,2u}){
  std::array<uint32_t,120> oldCurrent{},oldComparison{},newCurrent{},newComparison{};
  oldCurrent.fill(0xabababab);oldComparison.fill(0xcccccccc);
  StudioPixelColorOwnerCandidate owner{{0,0,0},false,false,1,oldCurrent.data(),oldComparison.data()};
  struct Services{
   StudioPixelColorOwnerCandidate& owner;unsigned selected,transforms=0,uploads=0;
   uint32_t* current;uint32_t* comparison;
   StudioPixelExposureCandidate exposure(){owner.channels={1,2,3};return {2,true};}
   unsigned mode(){owner.transformCaptured=true;return selected;}
   bool usePower(){return false;}
   float transform(float x){++transforms;if(transforms==1)owner.channels={10,20,30};return x+1;}
   double power(double,double){throw 2;}
   unsigned serviceMode(){owner.scale=2;return 1;}
   void submitPixel(unsigned index,unsigned count,const uint32_t*){
    if(index!=29||count!=1)throw 3;++uploads;owner.current=current;owner.comparison=comparison;
   }
  }services{owner,selected,0,0,newCurrent.data(),newComparison.data()};
  if(!studioPixelColorOwnerUploadCandidate(owner,services)||services.uploads!=1||services.transforms!=3)return 8;
  const std::array<float,4> expected=selected==1?std::array<float,4>{4,6,8,.5f}:std::array<float,4>{4,42,62,.5f};
  if(std::memcmp(newCurrent.data()+116,expected.data(),16)||std::memcmp(newComparison.data()+116,expected.data(),16))return 9;
  if(oldCurrent[116]!=0xabababab||oldComparison[116]!=0xcccccccc)return 10;
  ++mutationCases;
 }
 std::printf("pixel_color_owner_capture_live_scale_cache_mutation_cases=%u\n",mutationCases);
}
