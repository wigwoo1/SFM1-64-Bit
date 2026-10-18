#define NOMINMAX
#include <windows.h>
#include <cstring>
#include <array>
#include <iostream>
#include "studio_material_shared_buffer_initialization_candidate.h"
#include "studio_material_context_teardown_candidate.h"
#include "studio_material_buffer_release_candidate.h"
int main(){unsigned cases=0;
for(const char* game: {"sfm","infested","swarm","portal2","SFM","INFESTED","SwArM","PoRtAl2"}){StudioMaterialSharedBufferControlCandidate shared{0,99,99};std::array<StudioMaterialBufferCandidate,2> buffers;unsigned initialized=0,reads=0;uintptr_t reserve=studioMaterialGameKeyEqualCandidate(game,"portal2")?0x672000:studioMaterialGameKeyEqualCandidate(game,"sfm")?0x226000:0x44c000;auto rounded=(reserve+0x7fff)&~uintptr_t(0x7fff);
auto init=[&](unsigned index,uintptr_t bytes,uintptr_t alignment,uintptr_t initial,uintptr_t nodeAlignment){if(bytes!=reserve||alignment!=0x8000||initial||nodeAlignment!=32)throw 1;auto* allocation=VirtualAlloc(nullptr,rounded,MEM_RESERVE,PAGE_NOACCESS);if(!allocation)throw 1;auto base=reinterpret_cast<uintptr_t>(allocation);buffers[index]={base,base,base+rounded,base,false,alignment};++initialized;};
for(unsigned user=0;user<16;++user)studioMaterialSharedBuffersAcquireCandidate(shared,[&]{++reads;return game;},[](const char* left,const char* right){return studioMaterialGameKeyEqualCandidate(left,right);},init);
if(shared.users!=16||initialized!=2||shared.selector||shared.diagnostic||reads!=(studioMaterialGameKeyEqualCandidate(game,"infested")?1u:studioMaterialGameKeyEqualCandidate(game,"swarm")?2u:3u))return 1;
std::array<uintptr_t,2> bases{buffers[0].base,buffers[1].base};unsigned released=0;
for(unsigned user=0;user<16;++user){studioMaterialSharedBuffersReleaseCandidate(shared.users,[&](unsigned index){studioMaterialBufferReleaseCandidate(buffers[index],[](uintptr_t address,uintptr_t bytes,int type){return VirtualFree(reinterpret_cast<void*>(address),bytes,type)!=0;});++released;});if(released!=(user==15?2u:0u))return 1;}
for(auto base:bases){MEMORY_BASIC_INFORMATION info{};if(!VirtualQuery(reinterpret_cast<void*>(base),&info,sizeof(info))||info.State!=MEM_FREE)return 1;}++cases;
}std::cout<<"shared_buffer_game_cases="<<cases<<" actual_reservations=16 users=128 last_user_release=1\n";}
