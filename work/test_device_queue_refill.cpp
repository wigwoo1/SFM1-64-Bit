#include "studio_device_queue_refill_candidate.h"
#include <array>
#include <cstdio>
struct Ops{
 std::array<uint32_t,4097>& replacement;uint32_t* expected;
 unsigned publishes=0,acquires=0;
 void publish(uint32_t* block){if(block!=expected||block[0]!=2||block[3]!=0)throw 1;++publishes;}
 uint32_t* acquire(unsigned header){if(header!=1)throw 2;replacement[0]=header;++acquires;return replacement.data();}
};
int main(){
 static_assert(sizeof(void*)==8,"x64 required");unsigned cases=0;
 for(unsigned mode=0;mode<3;++mode){
  std::array<uint32_t,4097> old{},next{};old.fill(99);next.fill(88);old[0]=1;
  StudioDeviceQueueStateCandidate state{mode?old.data():nullptr,mode?old.data()+(mode==1?1:3):nullptr,6};
  Ops ops{next,old.data()};studioDeviceQueueRefillCandidate(state,ops);
  if(mode==1){if(ops.publishes||ops.acquires||state.block!=old.data()||state.cursor!=old.data()+1||state.remaining!=6||old[0]!=1)return 1;}
  else{
   if(ops.acquires!=1||ops.publishes!=unsigned(mode==2)||state.block!=next.data()||state.cursor!=next.data()+1||state.remaining!=4095)return 2;
   // Exercise the seven-word pixel packet after refill as in the native oracle.
   const std::array<uint32_t,7> packet{7,29,1,0,0,0,0x3f000000};
   for(auto word:packet)*state.cursor++=word;state.remaining-=7;
   if(state.cursor!=next.data()+8||state.remaining!=4088||next[0]!=1||next[8]!=88)return 3;
  }
  ++cases;
 }
 std::printf("x64_device_queue_refill_cases=%u absent_empty_nonempty=1 full_width_pointers=1\n",cases);
}
