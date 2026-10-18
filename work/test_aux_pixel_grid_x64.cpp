#include "studio_aux_pixel_constant_candidate.h"
#include "studio_aux_pixel_owner_candidate.h"
#include <fstream>
#include <cstdio>
#include <limits>
struct Ops{
 unsigned selected;bool equal;float value;unsigned published=0;
 void publishIndex(unsigned index){published=index;}
 unsigned mode(){if(published!=17)throw 1;return selected;}
 float lower(){return 2;}float upper(){return equal?2.f:6.f;}
 float second(){return 3;}float density(){return value;}
};
int main(){static_assert(sizeof(void*)==8,"x64 required");
 std::ifstream oracle("outputs/native-aux-pixel-grid.bin",std::ios::binary);if(!oracle)return 1;
 unsigned cases=0;uint64_t digest=1469598103934665603ull;
 for(unsigned mode=0;mode<2;++mode)for(unsigned enabled=0;enabled<2;++enabled)for(unsigned equal=0;equal<2;++equal)
 for(float density:{-1.f,.5f,2.f,std::numeric_limits<float>::quiet_NaN()}){
  Ops ops{mode,equal!=0,density};const auto result=studioAuxPixelConstantCandidate(17,enabled,ops);
  std::array<uint32_t,4> native{};oracle.read(reinterpret_cast<char*>(native.data()),16);if(!oracle)return 2;
  if(result!=native)return 3;
  std::array<uint32_t,72> current{},comparison{};comparison.fill(0xcccccccc);
  StudioAuxPixelOwnerCandidate owner{0xffffffffu,2,equal?2.f:6.f,3,density,current.data(),comparison.data()};
  struct Services{
   StudioAuxPixelOwnerCandidate& owner;unsigned selected,calls=0;
   unsigned mode(){if(owner.index!=17)throw 5;return selected;}
   void submitPixel(unsigned index,unsigned count,const uint32_t*){if(index!=17||count!=1)throw 6;++calls;}
  }services{owner,mode};
  if(!studioAuxPixelOwnerUploadCandidate(owner,17,enabled,services)||services.calls!=1)return 5;
  if(std::memcmp(current.data()+68,native.data(),16)||std::memcmp(comparison.data()+68,native.data(),16))return 6;
  if(studioAuxPixelOwnerUploadCandidate(owner,17,enabled,services)||services.calls!=1)return 7;
  for(auto word:result)digest=(digest^word)*1099511628211ull;++cases;
 }
 if(oracle.peek()!=std::char_traits<char>::eof())return 4;
 // Mode callback changes live calculation inputs. Submission replaces both
 // borrowed cache pointers; publication must target the replacements.
 std::array<uint32_t,72> oldCurrent{},oldComparison{},newCurrent{},newComparison{};
 oldCurrent.fill(0xabababab);oldComparison.fill(0xcccccccc);
 StudioAuxPixelOwnerCandidate changed{0xffffffffu,0,0,0,0,oldCurrent.data(),oldComparison.data()};
 struct MutatingServices{
  StudioAuxPixelOwnerCandidate& owner;uint32_t* current;uint32_t* comparison;unsigned calls=0;
  unsigned mode(){if(owner.index!=17)throw 7;owner.lower=2;owner.upper=6;owner.second=3;owner.density=.5f;return 1;}
  void submitPixel(unsigned index,unsigned count,const uint32_t* words){
   if(index!=17||count!=1||words[0]!=0xbf000000u||words[1]!=0x40400000u||words[2]!=0x3f000000u||words[3]!=0x3e800000u)throw 8;
   ++calls;owner.current=current;owner.comparison=comparison;
  }
 }mutating{changed,newCurrent.data(),newComparison.data()};
 if(!studioAuxPixelOwnerUploadCandidate(changed,17,1,mutating)||mutating.calls!=1)return 8;
 const std::array<uint32_t,4> expected{0xbf000000u,0x40400000u,0x3f000000u,0x3e800000u};
 if(std::memcmp(newCurrent.data()+68,expected.data(),16)||std::memcmp(newComparison.data()+68,expected.data(),16))return 9;
 if(oldCurrent[68]!=0xabababab||oldComparison[68]!=0xcccccccc)return 10;
 if(studioAuxPixelOwnerUploadCandidate(changed,17,1,mutating)||mutating.calls!=1)return 11;
 std::printf("x64_native_aux_pixel_cases=%u digest=%llu\n",cases,static_cast<unsigned long long>(digest));
 std::puts("aux_owner_live_callback_and_cache_replacement=passed");
}
