#include "studio_device_queue_publish_candidate.h"
#include <fstream>
#include <iostream>
static void check(bool b){if(!b)throw std::runtime_error("publication comparison mismatch");}
struct Services{StudioDeviceQueuePublishCandidate& state;unsigned waits=0;void waitZero(){++waits;check(waits==1);state.completed=state.produced-499u;}};
int main(){try{static_assert(sizeof(std::uintptr_t)==8,"x64 publication");std::ifstream records("outputs/native-restore-publication.bin",std::ios::binary);check(bool(records));unsigned cases=0;
 for(unsigned producer:{0u,499u,500u,0xffffffffu})for(unsigned index:{0u,1u,499u})for(unsigned full=0;full<2;++full){
  StudioDeviceQueuePublishCandidate state;state.ring.fill(0xabababab);state.produced=producer;state.index=index;state.completed=producer-(full?500u:0u);Services services{state};studioDeviceQueuePublishCandidate(state,0x12345678,services);
  std::array<unsigned,640> actual{},original{};actual.fill(0xabababab);for(unsigned i=0;i<500;++i)actual[i]=static_cast<unsigned>(state.ring[i]);actual[0x8d0/4]=state.produced;actual[0x8d4/4]=state.index;actual[0x9d8/4]=state.completed;unsigned originalWaits=0;records.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(records.gcount()==sizeof(original));records.read(reinterpret_cast<char*>(&originalWaits),4);check(records.gcount()==4);check(actual==original&&services.waits==originalWaits);++cases;
 }
 check(records.peek()==std::char_traits<char>::eof());StudioDeviceQueuePublishCandidate wide;Services services{wide};const std::uintptr_t handle=0x123456789abcdef0ull;studioDeviceQueuePublishCandidate(wide,handle,services);check(wide.ring[0]==handle&&wide.produced==1&&wide.index==1&&services.waits==0);
 std::cout<<"x64_publication_records_matched="<<cases<<" wide_handle_passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
