#include <vector>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include "studio_resource_startup_candidate.h"
static void check(bool b){if(!b)std::abort();}
struct Ops {
 StudioShutdownServicesCandidate& services;StudioResourceStartupCandidate& state;uintptr_t first,second;bool mutate;std::vector<unsigned> events;StudioResourceRegistrationNodeCandidate* a;StudioResourceRegistrationNodeCandidate* b;
 void precacheSlot10(uintptr_t p,uintptr_t group,unsigned i){check(p==((mutate&&i)?second:first)&&group==state.groups[i]);events.push_back(10+i);if(mutate)state.precache=second;}
 void resourceSlot8(uintptr_t p,uintptr_t token){check(p==first&&token==second&&!services.resourceRegistered);events.push_back(20);if(mutate)services.resource=second;}
 void resourceSlot12(uintptr_t p,StudioResourceRegistrationNodeCandidate* node){check(p==(mutate?second:first)&&services.resourceRegistered&&(node==a||node==b));events.push_back(node==a?30:31);if(mutate&&node==a)node->next=nullptr;}
};
int main(){static_assert(sizeof(void*)==8,"x64");unsigned objects[2]{};uintptr_t first=reinterpret_cast<uintptr_t>(&objects[0]),second=reinterpret_cast<uintptr_t>(&objects[1]);check(first>UINT32_MAX&&second>UINT32_MAX);unsigned cases=0;std::ifstream input("outputs/native-resource-startup.bin",std::ios::binary);
 for(bool precache:{false,true})for(bool initialized:{false,true})for(bool resource:{false,true})for(bool registered:{false,true})for(bool mutate:{false,true}){StudioResourceRegistrationNodeCandidate b,a{&b};StudioShutdownServicesCandidate services;services.resource=resource?first:0;services.resourceToken=second;services.resourceRegistered=registered;StudioResourceStartupCandidate state;state.precache=precache?first:0;state.precacheInitialized=initialized;state.head=&a;state.groups.fill(second);Ops ops{services,state,first,second,mutate,{},&a,&b};studioConnectResourceServicesCandidate(services,state,ops);std::vector<unsigned> expected;if(precache&&!initialized)for(unsigned i=0;i<5;++i)expected.push_back(10+i);if(resource&&!registered){expected.push_back(20);expected.push_back(30);if(!mutate)expected.push_back(31);}check(ops.events==expected&&state.precacheInitialized==(initialized||precache)&&services.resourceRegistered==(registered||resource)); auto tag=[&](uintptr_t p){check(p==0||p==first||p==second);return p==first?1u:p==second?2u:0u;};std::array<unsigned,16> wire{},original{};wire[0]=static_cast<unsigned>(ops.events.size());std::copy(ops.events.begin(),ops.events.end(),wire.begin()+1);wire[9]=unsigned(state.precacheInitialized);wire[10]=unsigned(services.resourceRegistered);wire[11]=tag(state.precache);wire[12]=tag(services.resource);wire[13]=a.next?1:0;input.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(input)&&wire==original);++cases;}
 check(input.peek()==std::char_traits<char>::eof());std::printf("x64_resource_startup_registration_cases=%u passed\n",cases);
}

