#include <array>
#include <cstdio>
#include <stdexcept>
#include "studio_extended_vertex_constants_candidate.h"
#include "native_manager_callback_registry_candidate.h"
static void require(bool p){if(!p)throw std::runtime_error("extended constant mismatch");}
static unsigned dispatched;static void callback(){++dispatched;}
struct Allocator:NativeManagerCallbackAllocatorCandidate{bool fail=false;void* allocate(std::size_t n){return fail?nullptr:NativeManagerCallbackAllocatorCandidate::allocate(n);}};
using Registry=NativeManagerCallbackRegistryCandidateT<Allocator>;
struct Services{
 Registry& registry;int32_t capacity;uint8_t flags=0;unsigned warnings=0;std::array<float,6148> values{};
 int32_t vertexFloatCapacity(){return capacity;}float* staging(){return values.data();}uint8_t& registrationFlags(){return flags;}
 void warnClipped(unsigned limit){require(limit==512);++warnings;}void enqueueExtendedConstants(){registry.append(callback);}
 void copy(float* destination,const float* source,uint32_t bytes){std::memmove(destination,source,bytes);}
};
int main(){try{unsigned cases=0;
 for(int32_t capacity:{-1,256,1535,1536,2048})for(int32_t start:{0,510,512}){
  Allocator allocator;Registry registry{allocator};Services services{registry,capacity};services.values.fill(-7);std::array<float,16> source{};source.fill(3);
  const bool changed=studioExtendedVertexConstantsCandidate(start,4,source.data(),services);require(changed==(capacity>=1536&&start<512));
  const unsigned written=capacity<1536?0:start==0?16:start==510?8:0;
  for(unsigned i=0;i<services.values.size();++i)require(services.values[i]==(i>=unsigned(start+1024)*4&&i<unsigned(start+1024)*4+written?3.0f:-7.0f));
  require(registry.size()==(changed?1:0)&&services.flags==(changed?2:0));require(services.warnings==(capacity>=1536&&start+4>512?1u:0u));
  if(changed){source.fill(9);studioExtendedVertexConstantsCandidate(start,4,source.data(),services);require(registry.size()==1&&services.values[unsigned(start+1024)*4]==9);}
  ++cases;
 }
 Allocator allocator;Registry registry{allocator};Services services{registry,1536};float source[4]{1,2,3,4};allocator.fail=true;bool caught=false;try{studioExtendedVertexConstantsCandidate(0,1,source,services);}catch(const std::bad_alloc&){caught=true;}
 require(caught&&registry.size()==0&&services.flags==0&&services.values[4096]==1);allocator.fail=false;require(studioExtendedVertexConstantsCandidate(0,1,source,services)&&registry.size()==1&&services.flags==2);dispatched=0;registry.dispatch();require(dispatched==1);++cases;
 caught=false;try{studioExtendedVertexConstantsCandidate(-1,1,source,services);}catch(const std::invalid_argument&){caught=true;}require(caught);++cases;
 std::printf("extended_vertex_constants_cases=%u capacity_clipping_retry=passed\n",cases);
 }catch(const std::exception& e){std::puts(e.what());return 1;}}
