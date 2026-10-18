#include <array>
#include <cstdio>
#include <cstdlib>
#include "studio_composed_base_shutdown_candidate.h"
static void check(bool b){if(!b)std::abort();}
struct Ops {
 StudioShutdownServicesCandidate& state;uintptr_t first,second;unsigned mode,trace=0;
 void deleteOwnedTree(uintptr_t){std::abort();}
 void resourceSlot9(uintptr_t p,uintptr_t token){check(p==first&&token==second);trace=1;state.resource=second;state.resourceRegistered=true;}
 void cvarSlot32(uintptr_t p,unsigned h){check(p==first&&h==7);trace=32;if(mode&1)state.cvar=second;if(mode&2)state.cvarHandle=9;}
 void cvarSlot11(uintptr_t p,unsigned h){check(p==((mode&1)?second:first)&&h==((mode&2)?9u:7u));trace=3211;state.cvarRegistered=true;state.cvarHandle=99;}
};
int main(){static_assert(sizeof(void*)==8,"x64");std::array<unsigned,2> providers{};uintptr_t first=reinterpret_cast<uintptr_t>(&providers[0]),second=reinterpret_cast<uintptr_t>(&providers[1]);check(first>UINT32_MAX&&second>UINT32_MAX);unsigned cases=0;
 for(bool present:{false,true})for(bool registered:{false,true})for(unsigned mode:{0u,1u,2u,3u}){
 StudioShutdownServicesCandidate state{present?first:0,present?first:0,second,7,registered,registered};Ops ops{state,first,second,mode};studioResourceShutdownCandidate(state,ops);check(ops.trace==unsigned(present&&registered));check(state.resource==((present&&registered)?second:(present?first:0)));check(state.resourceRegistered==(registered&&!present));ops.trace=0;studioCvarShutdownCandidate(state,ops);check(ops.trace==((present&&registered)?3211u:0u));check(state.cvar==((present&&registered&&(mode&1))?second:(present?first:0)));check(state.cvarHandle==((present&&registered)?0xffffffffu:7u));check(state.cvarRegistered==(registered&&!present));++cases;
 }
  StudioShutdownServicesCandidate serviceState;Ops emptyOps{serviceState,first,second,0};StudioDeviceManagerBaseShutdownCandidate base{first,second,0};StudioInterfaceConnectorCandidate interfaces;interfaces.connection=1;interfaces.tracked=1;uintptr_t cell=first;interfaces.records[0]={&cell,0};studioComposedBaseShutdownCandidate(base,serviceState,interfaces,emptyOps);check(base.service54==0&&base.service58==0&&cell==0&&interfaces.connection==0&&interfaces.tracked==1&&emptyOps.trace==0);
 std::printf("x64_shutdown_service_pointer_reload_cases=%u passed\n",cases);
}

