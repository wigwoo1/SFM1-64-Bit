#include "studio_pixel_constant_submit_candidate.h"
#include "studio_pixel_constant_tail_candidate.h"
#include "studio_aux_pixel_owner_candidate.h"
#include "studio_pixel_color_owner_candidate.h"
#include "studio_render_state_owner_candidate.h"
#include <cstdio>
struct Ops{
 bool queued=true;unsigned publishes=0,acquires=0,direct=0;
 unsigned expectedIndex=29;
 unsigned mode(){return 1;}
 StudioPixelExposureCandidate exposure(){return {2,true};}
 bool usePower(){return false;}float transform(float){throw 5;}
 double power(double,double){throw 6;}unsigned serviceMode(){return 0;}
 std::array<uint32_t,4097> old{},next{};
 std::array<uint32_t,120> current{},comparison{};
 StudioDeviceQueueStateCandidate state{};
 bool queueEnabled(){return queued;}auto& queueState(){return state;}
 void publish(uint32_t* block){if(block!=old.data()||block[0]!=2||block[3]!=0)throw 1;++publishes;}
 uint32_t* acquire(unsigned header){if(header!=1)throw 2;++acquires;next[0]=1;return next.data();}
 void directPixel(unsigned index,unsigned count,const uint32_t*){if(index!=expectedIndex||count!=1)throw 3;++direct;}
 uint32_t* currentCache(){return current.data();}uint32_t* comparisonCache(){return comparison.data();}
 void submitPixel(unsigned index,unsigned count,const uint32_t* words){
  if(index!=29||count!=1)throw 4;std::array<uint32_t,4> local{};std::memcpy(local.data(),words,16);
  studioPixelConstantSubmitCandidate(local,*this);
 }
};
int main(){unsigned cases=0;const std::array<uint32_t,4> local{0,0x7fc12345,0x3f800000,0x3f000000};
 for(unsigned mode=0;mode<3;++mode){
  Ops ops;ops.queued=mode!=0;ops.old.fill(99);ops.next.fill(88);ops.old[0]=1;
  ops.state={ops.old.data(),ops.old.data()+3,mode==2?6u:7u};
  if(!studioPixelConstantTailCandidate(local,ops))return 1;
  if(std::memcmp(ops.current.data()+116,local.data(),16)||std::memcmp(ops.comparison.data()+116,local.data(),16))return 2;
  if(mode){auto* packet=mode==2?ops.next.data()+1:ops.old.data()+3;
   if(packet[0]!=7||packet[1]!=29||packet[2]!=1||std::memcmp(packet+3,local.data(),16))return 3;
   if(ops.publishes!=unsigned(mode==2)||ops.acquires!=unsigned(mode==2)||ops.direct||ops.state.remaining!=(mode==2?4088u:0u))return 4;
  }else if(ops.direct!=1||ops.acquires||ops.publishes)return 5;
  const auto cursor=ops.state.cursor;
  if(studioPixelConstantTailCandidate(local,ops)||ops.state.cursor!=cursor)return 6;
  ++cases;
 }
 std::printf("x64_pixel_constant_submit_cases=%u direct_queue_rollover_cache_reuse=1\n",cases);
 unsigned ownerCases=0;
 for(unsigned mode=0;mode<3;++mode){
  Ops backend;backend.expectedIndex=17;backend.queued=mode!=0;
  backend.old.fill(99);backend.next.fill(88);backend.old[0]=1;
  backend.state={backend.old.data(),backend.old.data()+3,mode==2?6u:7u};
  backend.comparison.fill(0xcccccccc);
  StudioAuxPixelOwnerCandidate owner{0xffffffffu,2,6,3,.5f,backend.current.data(),backend.comparison.data()};
  StudioAuxPixelTransportCandidate<Ops> services{backend};
  const std::array<uint32_t,4> expected{0xbf000000,0x40400000,0x3f000000,0x3e800000};
  if(!studioAuxPixelOwnerUploadCandidate(owner,17,1,services)||owner.index!=17)return 7;
  if(std::memcmp(backend.current.data()+68,expected.data(),16)||std::memcmp(backend.comparison.data()+68,expected.data(),16))return 8;
  if(mode){auto* packet=mode==2?backend.next.data()+1:backend.old.data()+3;
   if(packet[0]!=7||packet[1]!=17||packet[2]!=1||std::memcmp(packet+3,expected.data(),16))return 9;
   if(backend.direct||backend.publishes!=unsigned(mode==2)||backend.acquires!=unsigned(mode==2)||backend.state.remaining!=(mode==2?4088u:0u))return 10;
  }else if(backend.direct!=1||backend.publishes||backend.acquires)return 11;
  auto* cursor=backend.state.cursor;
  if(studioAuxPixelOwnerUploadCandidate(owner,17,1,services)||backend.state.cursor!=cursor||backend.direct!=unsigned(mode==0))return 12;
  ++ownerCases;
 }
 std::printf("aux_owner_transport_direct_exact_capacity_rollover_cases=%u\n",ownerCases);
 unsigned colorCases=0;
 for(unsigned mode=0;mode<3;++mode){
  Ops backend;backend.queued=mode!=0;backend.old.fill(99);backend.next.fill(88);backend.old[0]=1;
  backend.state={backend.old.data(),backend.old.data()+3,mode==2?6u:7u};backend.comparison.fill(0xcccccccc);
  StudioPixelColorOwnerCandidate owner{{1,2,3},false,false,1,backend.current.data(),backend.comparison.data()};
  StudioPixelColorTransportCandidate<Ops> services{backend};
  const std::array<float,4> expected{1,2,3,.5f};
  if(!studioPixelColorOwnerUploadCandidate(owner,services))return 13;
  if(std::memcmp(backend.current.data()+116,expected.data(),16)||std::memcmp(backend.comparison.data()+116,expected.data(),16))return 14;
  if(mode){auto* packet=mode==2?backend.next.data()+1:backend.old.data()+3;
   if(packet[0]!=7||packet[1]!=29||packet[2]!=1||std::memcmp(packet+3,expected.data(),16))return 15;
   if(backend.direct||backend.publishes!=unsigned(mode==2)||backend.acquires!=unsigned(mode==2)||backend.state.remaining!=(mode==2?4088u:0u))return 16;
  }else if(backend.direct!=1||backend.publishes||backend.acquires)return 17;
  auto* cursor=backend.state.cursor;
  if(studioPixelColorOwnerUploadCandidate(owner,services)||backend.state.cursor!=cursor||backend.direct!=unsigned(mode==0))return 18;
  ++colorCases;
 }
 std::printf("color_owner_transport_direct_exact_capacity_rollover_cases=%u\n",colorCases);
 unsigned renderCases=0;
 for(unsigned mode=0;mode<3;++mode){
  Ops backend;backend.queued=mode!=0;backend.old.fill(99);backend.next.fill(88);backend.old[0]=1;
  backend.state={backend.old.data(),backend.old.data()+3,mode==2?2u:3u};
  std::array<uint32_t,64> cache{};cache.fill(0xcccccccc);
  StudioRenderStateOwnerCandidate owner{cache.data(),&backend.state};
  struct Services{
   Ops& backend;unsigned direct=0,flushes=0;
   unsigned suppressed(){return 0;}void flush(){++flushes;}
   bool queueEnabled(){return backend.queued;}
   void directState(unsigned key,unsigned value){if(key!=28||value!=1)throw 7;++direct;}
   void publish(uint32_t* block){backend.publish(block);}
   uint32_t* acquire(unsigned header){return backend.acquire(header);}
  }services{backend};
  studioRenderStateOwnerSubmitCandidate(owner,28,1,0x101,services);
  if(cache[28]!=1||services.flushes!=1)return 19;
  if(mode){auto* packet=mode==2?backend.next.data()+1:backend.old.data()+3;
   if(packet[0]!=1||packet[1]!=28||packet[2]!=1)return 20;
   if(services.direct||backend.publishes!=unsigned(mode==2)||backend.acquires!=unsigned(mode==2)||backend.state.remaining!=(mode==2?4092u:0u))return 21;
  }else if(services.direct!=1||backend.publishes||backend.acquires)return 22;
  auto* cursor=backend.state.cursor;
  studioRenderStateOwnerSubmitCandidate(owner,28,1,1,services);
  if(backend.state.cursor!=cursor||services.flushes!=1||services.direct!=unsigned(mode==0))return 23;
  ++renderCases;
 }
 std::printf("render_state_owner_direct_exact_capacity_rollover_cases=%u\n",renderCases);
}
