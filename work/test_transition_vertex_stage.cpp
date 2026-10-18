#include "studio_transition_vertex_stage_candidate.h"
#include "studio_transition_vertex_owner_candidate.h"
#include "studio_transition_owner_entry_candidate.h"
#include "studio_vertex_constant_submit_candidate.h"
#include "studio_shader_transition_candidate.h"
#include "studio_slot267_candidate.h"
#include "studio_aux_pixel_constant_candidate.h"
#include "studio_aux_pixel_owner_candidate.h"
#include "studio_pixel_color_candidate.h"
#include "studio_pixel_color_owner_candidate.h"
#include "studio_shader_color_conversion_candidate.h"
#include "studio_shader_color_conversion_owner_candidate.h"
#include "studio_color_lookup_candidate.h"
#include "studio_application_math_candidate.h"
#include "studio_owned_color_lookup_candidate.h"
#include "studio_shader_render_state_submit_candidate.h"
#include "studio_render_state_owner_candidate.h"
#include <cstdio>
#include <fstream>
struct Ops{
 unsigned state=0,entry=0,capCalls=0,uploads=0,colorReads=0;int level=90;
 bool mutation=false;float hi=6,densityValue=.5f;
 std::array<uint32_t,80> primary{},secondary{};
 unsigned bytes=0,aux=0,pixels=0,renders=0,byteValue=99,auxEnable=99;uint32_t packed=0;
 bool disabled=false;unsigned conversions=0;std::array<float,3> normalized{};
 uint8_t byteCache=255;unsigned byteRenders=0;
 std::array<uint32_t,120> pixelCurrent{},pixelComparison{};unsigned auxUploads=0;
 unsigned colorUploads=0;
 std::array<uint32_t,64> renderCaches{};
 bool queued=false;unsigned publishes=0,acquires=0,publishedWords=0;
 std::array<uint32_t,4097> queueOld{},queueNext{};StudioDeviceQueueStateCandidate commandState{};
 bool queueEnabled(){return queued;}auto& queueState(){return commandState;}
 void publish(uint32_t* block){if(block!=queueOld.data()||block[0]!=2||*commandState.cursor!=0)throw 14;publishedWords=static_cast<unsigned>(commandState.cursor-block-3);++publishes;}
 uint32_t* acquire(unsigned header){if(header!=1)throw 15;++acquires;queueNext[0]=1;return queueNext.data();}
 void directPixel(unsigned,unsigned,const uint32_t*){}
 void directVertex(unsigned,unsigned,const uint32_t*){}
 void pixelSubmit(unsigned index,const uint32_t* words){std::array<uint32_t,4> local{};std::memcpy(local.data(),words,16);studioPixelConstantSubmitCandidate(index,local,*this);}
 void transitionByte(unsigned value){++bytes;byteValue=value;studioSlot267Candidate(value,byteCache,*this);}
 unsigned auxiliaryIndex(){return 17;}
 void auxiliary(unsigned index,unsigned enable){if(index!=17)throw 3;++aux;auxEnable=enable;
  StudioAuxPixelOwnerCandidate normalizedOwner{0xffffffffu,2,hi,4,densityValue,pixelCurrent.data(),pixelComparison.data()};
  struct Services{Ops& owner;StudioAuxPixelOwnerCandidate& normalizedOwner;
   unsigned mode(){if(normalizedOwner.index!=17)throw 12;return 1;}
   void submitPixel(unsigned index,unsigned count,const uint32_t* words){if(index!=17||count!=1)throw 6;++owner.auxUploads;owner.pixelSubmit(index,words);}
  }services{*this,normalizedOwner};studioAuxPixelOwnerUploadCandidate(normalizedOwner,index,enable,services);
 }
 std::array<uint8_t,3> liveColor(){if(aux!=1)throw 4;return {64,96,192};}
 void publishConversionDisabled(bool value){disabled=value;}
 uint32_t convert(std::array<uint8_t,3> rgb,unsigned flag){++conversions;
  StudioShaderColorConversionOwnerCandidate normalizedOwner{1,0,1,1};
  struct Services{Ops& owner;unsigned mode(){++owner.entry;return 0;}
   float lookup(float value){studioInitializeApplicationMathCandidate();StudioOwnedColorLookupCandidate lookup{studioApplicationMathOwnerCandidate()};return lookup.lookup(value);}
  }services{*this};return studioShaderColorConversionOwnerCandidate(normalizedOwner,rgb,flag,services);
 }
 void publishNormalized(std::array<float,3> value){normalized=value;}
 void pixelHelper(){++pixels;
  StudioPixelColorOwnerCandidate normalizedOwner{normalized,false,disabled,1,pixelCurrent.data(),pixelComparison.data()};
  struct Services{Ops& owner;StudioPixelExposureCandidate exposure(){return {2,true};}unsigned mode(){return 1;}
   bool usePower(){return false;}
   float transform(float){throw 7;}double power(double,double){throw 8;}unsigned serviceMode(){++owner.entry;return 0;}
   void submitPixel(unsigned index,unsigned count,const uint32_t* words){if(index!=29||count!=1)throw 9;++owner.colorUploads;owner.pixelSubmit(index,words);}
  }services{*this};studioPixelColorOwnerUploadCandidate(normalizedOwner,services);
 }uint32_t packedCache(){return packed;}
 void publishPacked(uint32_t value){packed=value;}
 void renderState(unsigned index,unsigned value,unsigned force){if(force||!(index==28||index==34))throw 5;
  StudioRenderStateOwnerCandidate normalizedOwner{renderCaches.data(),&commandState};
  struct Services{Ops& owner;unsigned suppressed(){return 0;}void flush(){throw 10;}bool queueEnabled(){return owner.queued;}
   void publish(uint32_t* block){owner.publish(block);}uint32_t* acquire(unsigned header){return owner.acquire(header);}
   void directState(unsigned index,unsigned){if(index==28)++owner.byteRenders;else ++owner.renders;}
  }services{*this};studioRenderStateOwnerSubmitCandidate(normalizedOwner,index,value,force,services);
 }
 void entryService(){++entry;}unsigned currentState(){if(entry!=1)throw 1;return state;}
 int capability(){++capCalls;if(mutation){if(capCalls==1)state=state?0:1;else{hi=100;densityValue=.25f;}}return level;}
 float upper(){return hi;}float lower(){return 2;}float density(){return densityValue;}
 std::array<uint32_t,4> vertexColor(){++colorReads;return {0x3f800000,0x40000000,0x40400000,0x40800000};}
 uint32_t* primaryCache(){return primary.data();}uint32_t* secondaryCache(){return secondary.data();}
 void submit(unsigned index,unsigned count,const uint32_t* words){if(index!=(uploads?2u:16u)||count!=1)throw 2;++uploads;
  std::array<uint32_t,4> local{};std::memcpy(local.data(),words,16);studioVertexConstantSubmitCandidate(index,local,*this);
 }
};
int main(){unsigned cases=0;
 std::ifstream oracle("outputs/native-shader-transition-grid.bin",std::ios::binary);if(!oracle)return 5;
 std::ifstream queueGrid("outputs/native-shader-transition-queue-grid.bin",std::ios::binary);if(!queueGrid)return 18;
 std::ifstream rolloverGrid("outputs/native-shader-transition-rollover-grid.bin",std::ios::binary);if(!rolloverGrid)return 21;
 std::ifstream splitGrid("outputs/native-shader-transition-split-grid.bin",std::ios::binary);if(!splitGrid)return 24;
 for(unsigned input=0;input<7;++input)for(unsigned state=0;state<3;++state)for(unsigned modern=0;modern<2;++modern)for(unsigned flag=0;flag<2;++flag)for(unsigned mutation=0;mutation<2;++mutation){
  Ops ops;ops.state=state;ops.level=modern?91:90;ops.mutation=mutation!=0;
  ops.primary.fill(0xcccccccc);
  float lower=2;uint32_t* primary=ops.primary.data();uint32_t* secondary=ops.secondary.data();
  StudioTransitionVertexOwnerCandidate vertexOwner{ops.state,lower,ops.hi,ops.densityValue,{primary,secondary}};
  const auto selected=studioTransitionVertexOwnerCandidate(vertexOwner,input,flag?0x101u:0x100u,ops);
  const bool early=input==6||state==2,skip=early||(modern&&!flag);
  if(selected!=(skip?2u:modern?1u:0u)||ops.entry!=1||ops.uploads!=(skip?0u:2u)||ops.colorReads!=(skip?0u:1u)||ops.capCalls!=(early?0u:skip?1u:2u))return 1;
  if(!skip){if(std::memcmp(ops.primary.data()+64,ops.secondary.data()+64,16)||std::memcmp(ops.primary.data()+8,ops.secondary.data()+8,16))return 2;}
  Ops integrated;integrated.state=state;integrated.level=modern?91:90;integrated.mutation=mutation!=0;
  integrated.primary.fill(0xcccccccc);
  integrated.pixelComparison.fill(0xcccccccc);
  integrated.renderCaches.fill(0xcccccccc);
  uint32_t* integratedPrimary=integrated.primary.data();uint32_t* integratedSecondary=integrated.secondary.data();
  StudioTransitionVertexOwnerCandidate integratedOwner{integrated.state,lower,integrated.hi,integrated.densityValue,{integratedPrimary,integratedSecondary}};
  studioTransitionOwnerEntryCandidate(integratedOwner,input,flag?0x101u:0x100u,0x101,0,integrated);
  if(integrated.uploads!=ops.uploads||integrated.bytes!=1||integrated.aux!=1||integrated.auxEnable!=input)return 3;
  if(integrated.byteValue!=unsigned(input&&selected==0&&integrated.state!=0)||integrated.pixels!=unsigned(input&&selected!=0)||integrated.renders!=unsigned(input&&selected==0))return 4;
  std::array<uint32_t,32> native{},record{};oracle.read(reinterpret_cast<char*>(native.data()),sizeof(native));if(!oracle)return 6;
  if(std::memcmp(ops.primary.data()+64,native.data(),16)||std::memcmp(ops.primary.data()+8,native.data()+4,16))return 8;
  std::memcpy(record.data(),integrated.primary.data()+64,16);std::memcpy(record.data()+4,integrated.primary.data()+8,16);std::memcpy(record.data()+8,integrated.normalized.data(),12);
  record[11]=integrated.disabled;record[12]=integrated.packed;record[13]=integrated.byteValue;record[14]=integrated.pixels;record[15]=integrated.renders;record[16]=integrated.uploads;record[17]=integrated.conversions;
  record[18]=integrated.byteCache;record[19]=integrated.byteRenders;
  std::memcpy(record.data()+20,integrated.pixelCurrent.data()+68,16);record[24]=integrated.auxUploads;
  std::memcpy(record.data()+25,integrated.pixelCurrent.data()+116,16);record[29]=integrated.colorUploads;
  record[30]=integrated.renderCaches[28];record[31]=integrated.renderCaches[34];
  if(record!=native){std::printf("transition_state_mismatch_case=%u\n",cases);return 7;}
  Ops queued;queued.queued=true;queued.state=state;queued.level=modern?91:90;queued.mutation=mutation!=0;
  queued.primary.fill(0xcccccccc);queued.pixelComparison.fill(0xcccccccc);queued.renderCaches.fill(0xcccccccc);
  queued.queueOld.fill(0xcccccccc);queued.queueOld[0]=1;queued.commandState={queued.queueOld.data(),queued.queueOld.data()+1,64};
  uint32_t* queuedPrimary=queued.primary.data();uint32_t* queuedSecondary=queued.secondary.data();
  StudioTransitionVertexOwnerCandidate queuedOwner{queued.state,lower,queued.hi,queued.densityValue,{queuedPrimary,queuedSecondary}};
  studioTransitionOwnerEntryCandidate(queuedOwner,input,flag?0x101u:0x100u,0x101,0,queued);
  std::array<uint32_t,32> originalPacket{};queueGrid.read(reinterpret_cast<char*>(originalPacket.data()),sizeof(originalPacket));
  if(!queueGrid||std::memcmp(queued.queueOld.data()+1,originalPacket.data(),sizeof(originalPacket)))return 19;
  const unsigned used=(skip?0u:14u)+10u+(input?(selected?7u:3u):0u);
  if(queued.commandState.cursor!=queued.queueOld.data()+1+used||queued.commandState.remaining!=64-used||queued.publishes||queued.acquires||queued.renders||queued.byteRenders)return 20;
  const auto baselinePacket=originalPacket;
  for(unsigned capacity=0;capacity<32;++capacity){
  Ops rolled;rolled.queued=true;rolled.state=state;rolled.level=modern?91:90;rolled.mutation=mutation!=0;
  rolled.primary.fill(0xcccccccc);rolled.pixelComparison.fill(0xcccccccc);rolled.renderCaches.fill(0xcccccccc);rolled.queueNext.fill(0xcccccccc);
  rolled.queueOld[0]=1;rolled.commandState={rolled.queueOld.data(),rolled.queueOld.data()+3,capacity};
  uint32_t* rolledPrimary=rolled.primary.data();uint32_t* rolledSecondary=rolled.secondary.data();
  StudioTransitionVertexOwnerCandidate rolledOwner{rolled.state,lower,rolled.hi,rolled.densityValue,{rolledPrimary,rolledSecondary}};
  studioTransitionOwnerEntryCandidate(rolledOwner,input,flag?0x101u:0x100u,0x101,0,rolled);
  if(capacity==2){rolloverGrid.read(reinterpret_cast<char*>(originalPacket.data()),sizeof(originalPacket));
   if(!rolloverGrid||std::memcmp(rolled.queueNext.data()+1,originalPacket.data(),sizeof(originalPacket)))return 22;}
  uint32_t nativePrefix=0;splitGrid.read(reinterpret_cast<char*>(&nativePrefix),4);
  const bool refill=capacity<used;const unsigned prefix=refill?rolled.publishedWords:used,suffix=used-prefix;
  if(!splitGrid||prefix!=nativePrefix||std::memcmp(rolled.queueOld.data()+3,baselinePacket.data(),prefix*4)||std::memcmp(rolled.queueNext.data()+1,baselinePacket.data()+prefix,suffix*4))return 25;
  auto* expectedCursor=refill?rolled.queueNext.data()+1+suffix:rolled.queueOld.data()+3+used;
  if(rolled.publishes!=unsigned(refill)||rolled.acquires!=unsigned(refill)||rolled.commandState.cursor!=expectedCursor||rolled.commandState.remaining!=(refill?4095-suffix:capacity-used))return 23;
  }
  ++cases;
 }
 std::printf("transition_vertex_stage_cases=%u composed_transition_cases=%u native_skip_gates=1 paired_uploads=1\n",cases,cases);
 std::printf("native_composed_transition_queue_grid_cases=%u\n",cases);
 std::printf("native_transition_capacity_boundary_matches=%u\n",cases*32);
 unsigned queuedCases=0;
 for(unsigned modern=0;modern<2;++modern)for(unsigned rollover=0;rollover<2;++rollover){
  Ops ops;ops.state=1;ops.level=modern?91:90;ops.queued=true;
  ops.pixelComparison.fill(0xcccccccc);ops.renderCaches.fill(0xcccccccc);
  ops.queueOld[0]=1;ops.commandState={ops.queueOld.data(),ops.queueOld.data()+3,rollover?2u:4093u};
  float lower=2;uint32_t* primary=ops.primary.data();uint32_t* secondary=ops.secondary.data();
  StudioTransitionVertexOwnerCandidate owner{ops.state,lower,ops.hi,ops.densityValue,{primary,secondary}};
  studioTransitionOwnerEntryCandidate(owner,4,1,0x101,0,ops);
  auto* packet=rollover?ops.queueNext.data()+1:ops.queueOld.data()+3;
  if(packet[0]!=10||packet[1]!=16||packet[2]!=1||std::memcmp(packet+3,ops.primary.data()+64,16))return 14;
  if(packet[7]!=10||packet[8]!=2||packet[9]!=1||std::memcmp(packet+10,ops.primary.data()+8,16))return 15;
  auto* first=packet;packet+=14;
  if(packet[0]!=1||packet[1]!=28||packet[2]!=(modern?0u:1u))return 9;
  if(packet[3]!=7||packet[4]!=17||packet[5]!=1||std::memcmp(packet+6,ops.pixelCurrent.data()+68,16))return 10;
  if(modern){if(packet[10]!=7||packet[11]!=29||packet[12]!=1||std::memcmp(packet+13,ops.pixelCurrent.data()+116,16))return 11;}
  else if(packet[10]!=1||packet[11]!=34||packet[12]!=ops.packed)return 12;
  const unsigned used=modern?31u:27u;
  std::ifstream queueOracle("outputs/native-shader-transition-queue.bin",std::ios::binary);if(!queueOracle)return 16;
  std::array<uint32_t,32> originalQueue{};queueOracle.seekg(modern*32*4);queueOracle.read(reinterpret_cast<char*>(originalQueue.data()),sizeof(originalQueue));
  if(!queueOracle||std::memcmp(first,originalQueue.data(),used*4))return 17;
  if(ops.commandState.cursor!=first+used||ops.commandState.remaining!=(rollover?4095u:4093u)-used||ops.publishes!=rollover||ops.acquires!=rollover||ops.byteRenders||ops.renders)return 13;
  ++queuedCases;
 }
 std::printf("composed_transition_pixel_render_queue_cases=%u\n",queuedCases);
}

