#include "studio_pixel_color_candidate.h"
#include <cstdio>
struct Ops{
 unsigned selected=0,calls=0;bool captured=false,powerFlag=false,suppress=false;
 std::array<float,3> rgb{1,2,3};
 std::array<uint32_t,120> current{},comparison{};
 unsigned uploads=0;std::array<uint32_t,4> submitted{};
 uint32_t* comparisonCache(){return comparison.data();}
 uint32_t* currentCache(){return current.data();}
 void submitPixel(uint32_t index,uint32_t count,const uint32_t* words){
  if(index!=29||count!=1)throw 3;
  std::memcpy(submitted.data(),words,16);++uploads;
 }
 StudioPixelExposureCandidate exposure(){return {2,true};}
 unsigned mode(){return selected;}
 float channel(unsigned i){return rgb[i];}
 bool transformCaptured(){return captured;}
 bool usePower(){return powerFlag;}
 float transform(float x){++calls;rgb[1]=20;rgb[2]=30;return x+1;}
 double power(double x,double exponent){if(exponent!=2.200000047683716)throw 1;++calls;rgb[1]=20;rgb[2]=30;return x+2;}
 bool suppressScale(){return suppress;}
 unsigned serviceMode(){return 1;}
 float scale(){return 2;}
};
int main(){unsigned cases=0;
 for(unsigned mode=0;mode<3;++mode)for(unsigned captured=0;captured<2;++captured)
 for(unsigned power=0;power<2;++power)for(unsigned suppress=0;suppress<2;++suppress){
  Ops ops;ops.selected=mode;ops.captured=captured!=0;ops.powerFlag=power!=0;ops.suppress=suppress!=0;
  const auto result=studioPixelColorCandidate(ops);
  std::array<float,4> expected{0,0,0,.5f};
  unsigned calls=0;
  if(mode==1){expected[0]=1+float(captured);expected[1]=2+float(captured);expected[2]=3+float(captured);calls=captured?3:0;}
  if(mode==2){const float add=power?2.f:1.f;expected[0]=1+add;expected[1]=20+add;expected[2]=30+add;calls=3;}
  if(mode&&!suppress)for(unsigned i=0;i<3;++i)expected[i]*=2;
  if(result!=expected||ops.calls!=calls)return 2;
  ops.rgb={1,2,3};
  if(!studioPixelColorUploadCandidate(ops)||ops.uploads!=1)return 4;
  std::array<uint32_t,4> expectedBits{};
  std::memcpy(expectedBits.data(),expected.data(),16);
  if(ops.submitted!=expectedBits||std::memcmp(ops.current.data()+116,expectedBits.data(),16)
   ||std::memcmp(ops.comparison.data()+116,expectedBits.data(),16))return 5;
  ops.rgb={1,2,3};
  if(studioPixelColorUploadCandidate(ops)||ops.uploads!=1)return 6;
  ++cases;
 }
 std::printf("pixel_color_cases=%u captured_vs_live_callback_channels=1 combined_upload_cases=%u repeated_upload_skipped=1\n",cases,cases);
}
