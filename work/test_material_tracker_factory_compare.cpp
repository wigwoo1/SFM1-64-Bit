#include <array>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include "studio_material_interface_factory_candidate.h"
static void check(bool b){if(!b)std::abort();}
struct Ops{bool present,offer;const char* expectedName;std::array<uintptr_t,3> interfaces;uintptr_t upstreamObject,localObject;unsigned upstreamCalls=0,localCalls=0;
 uintptr_t direct(unsigned i){return interfaces[i];}bool hasUpstream(){return present;}
 uintptr_t upstream(const char* name,int* status){check(name==expectedName&&status==nullptr);++upstreamCalls;return offer?upstreamObject:0;}
 uintptr_t create(const StudioInterfaceFactoryNodeCandidate&){++localCalls;return localObject;}
};
int main(){static_assert(sizeof(void*)==8,"x64");std::ifstream input("outputs/native-material-tracker-factory.bin",std::ios::binary);const char* names[]={"VBAllocTracker001","missing","VFileSystem017","vfilesystem017","QueuedLoaderVersion001","queuedloaderversion001","VShaderUtil001","vshaderutil001"};std::array<unsigned,5> objects{};for(auto& o:objects)check(reinterpret_cast<uintptr_t>(&o)>UINT32_MAX);unsigned cases=0;
 for(bool present:{false,true})for(bool offer:{false,true})for(bool local:{false,true})for(unsigned nameCase=0;nameCase<8;++nameCase)for(bool statusPresent:{false,true})for(unsigned mask=0;mask<8;++mask){Ops ops{present,offer,names[nameCase],{},reinterpret_cast<uintptr_t>(&objects[3]),reinterpret_cast<uintptr_t>(&objects[4])};for(unsigned i=0;i<3;++i)ops.interfaces[i]=mask&(1u<<i)?reinterpret_cast<uintptr_t>(&objects[i]):0;StudioInterfaceFactoryNodeCandidate node{"VBAllocTracker001",0,nullptr};int status=99;auto returned=studioMaterialInterfaceFactoryCandidate(local?&node:nullptr,names[nameCase],statusPresent?&status:nullptr,ops);bool recognized=nameCase>=2;unsigned index=recognized?(nameCase-2)/2:0;bool upstreamSuccess=!recognized&&present&&offer,localSuccess=!recognized&&!upstreamSuccess&&local&&nameCase==0;check(returned==(recognized?ops.interfaces[index]:upstreamSuccess?ops.upstreamObject:localSuccess?ops.localObject:0));std::array<unsigned,4> actual{recognized?(ops.interfaces[index]?4u+index:0u):upstreamSuccess?1u:localSuccess?2u:0u,static_cast<unsigned>(status),ops.upstreamCalls,ops.localCalls},original{};input.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(input)&&actual==original);++cases;}
 check(input.peek()==std::char_traits<char>::eof());std::printf("x64_material_tracker_factory_records_matched=%u passed\n",cases);
}
