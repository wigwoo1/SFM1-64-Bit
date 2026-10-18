#include "studio_decal_pointer_frontier_candidate.h"
#include <cstdio>
#include <cstdlib>
void check(bool x){if(!x)std::abort();}
int main(){StudioDecalPointerNodeCandidate first[8],second[4];StudioDecalPointerBlockCandidate b{nullptr,4,second},a{&b,8,first};StudioDecalPointerPoolCandidate pool;unsigned cases=0;uintptr_t start=reinterpret_cast<uintptr_t>(first),older=reinterpret_cast<uintptr_t>(second);
 for(int index=-1;index<=8;++index){pool.frontier={&a,index};bool valid=index>=0&&index<8;
  for(unsigned offset=0;offset<sizeof(first);++offset){unsigned comparisons=0;auto* node=reinterpret_cast<StudioDecalPointerNodeCandidate*>(start+offset);bool result=studioDecalPointerBeyondFrontierCandidate(pool,node,[&](uintptr_t address,uintptr_t frontier){++comparisons;check(address==start+offset);return address>frontier;});check(result==(valid&&offset>unsigned(index)*sizeof(*first)));check(comparisons==unsigned(valid));++cases;}
  for(unsigned offset=0;offset<sizeof(second);++offset){check(studioDecalPointerBeyondFrontierCandidate(pool,reinterpret_cast<StudioDecalPointerNodeCandidate*>(older+offset),[](uintptr_t,uintptr_t)->bool{std::abort();})==valid);++cases;}
 }
 pool.frontier={&a,3};check(!studioDecalPointerBeyondFrontierCandidate(pool,nullptr,[](uintptr_t,uintptr_t)->bool{std::abort();}));check(studioDecalPointerBeyondFrontierCandidate(pool,first,[](uintptr_t,uintptr_t){return true;}));
 std::printf("PASS:%u full-width frontier states including unaligned interior addresses/invalid indices/later blocks and explicit ordering service. Native32-bit address parity pending.\n",cases);
}
