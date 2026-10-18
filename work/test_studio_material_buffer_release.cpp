#define NOMINMAX
#include <windows.h>
#include <iostream>
#include "studio_material_buffer_release_candidate.h"
int main(){for(unsigned committed=0;committed<2;++committed){auto* allocation=VirtualAlloc(nullptr,65536,MEM_RESERVE,PAGE_NOACCESS);if(!allocation)return 1;if(committed&&!VirtualAlloc(allocation,4096,MEM_COMMIT,PAGE_READWRITE))return 1;auto base=reinterpret_cast<uintptr_t>(allocation);StudioMaterialBufferCandidate buffer{base+committed*128,base+committed*4096,base+65536,base,committed!=0,4096};unsigned calls=0;bool valid=true;
studioMaterialBufferReleaseCandidate(buffer,[&](uintptr_t address,uintptr_t bytes,int type){++calls;if(address!=base||(type==MEM_DECOMMIT?bytes!=4096:bytes!=0))valid=false;return VirtualFree(reinterpret_cast<void*>(address),bytes,type)!=0;});
MEMORY_BASIC_INFORMATION info{};if(!VirtualQuery(allocation,&info,sizeof(info))||info.State!=MEM_FREE||buffer.base||calls!=1+committed||!valid||buffer.reservedEnd!=base+65536)return 1;
studioMaterialBufferReleaseCandidate(buffer,[&](uintptr_t,uintptr_t,int){++calls;return false;});if(calls!=1+committed)return 1;
}std::cout<<"buffer_release_cases=2 actual_reservations_freed=2 repeated_release_noop=1\n";}
