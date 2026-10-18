#include <array>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include "studio_interface_factory_fallback_candidate.h"
static void check(bool b){if(!b)std::abort();}
struct Ops{bool provided;const char* expectedName;int* expectedStatus;uintptr_t object,duplicate,externalObject;unsigned calls=0;
 uintptr_t external(const char* name,int* status){check(name==expectedName&&status==expectedStatus);++calls;if(status){check(*status==0);*status=77;}return provided?externalObject:0;}
 uintptr_t create(const StudioInterfaceFactoryNodeCandidate& node){return node.token?duplicate:object;}
};
int main(){static_assert(sizeof(void*)==8,"x64");std::ifstream input("outputs/native-tracker-factory.bin",std::ios::binary);unsigned cases=0;std::array<unsigned,3> objects{};for(auto& o:objects)check(reinterpret_cast<uintptr_t>(&o)>UINT32_MAX);
 for(bool provided:{false,true})for(bool local:{false,true})for(unsigned nameCase=0;nameCase<3;++nameCase)for(bool statusPresent:{false,true})for(bool duplicate:{false,true}){bool match=nameCase==0;StudioInterfaceFactoryNodeCandidate node{"VBAllocTracker001",0,nullptr},dup{"VBAllocTracker001",1,&node};const char* name=match?"VBAllocTracker001":nameCase==1?"missing":"vballoctracker001";int status=99;Ops ops{provided,name,statusPresent?&status:nullptr,reinterpret_cast<uintptr_t>(&objects[0]),reinterpret_cast<uintptr_t>(&objects[1]),reinterpret_cast<uintptr_t>(&objects[2])};auto returned=studioInterfaceFactoryFallbackCandidate(local?(duplicate?&dup:&node):nullptr,name,ops.expectedStatus,ops);check(returned==(provided?ops.externalObject:local&&match?(duplicate?ops.duplicate:ops.object):0)&&node.next==nullptr&&dup.next==&node);std::array<unsigned,3> actual{provided?1u:local&&match?(duplicate?3u:2u):0u,static_cast<unsigned>(status),ops.calls},original{};input.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(input)&&actual==original);++cases;}
 check(input.peek()==std::char_traits<char>::eof());std::printf("x64_tracker_factory_graph_records_matched=%u passed\n",cases);
}
