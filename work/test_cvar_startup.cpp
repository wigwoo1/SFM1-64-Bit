#include <array>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include "studio_cvar_startup_candidate.h"
static void check(bool b){if(!b)std::abort();}
struct Ops {
 StudioShutdownServicesCandidate& services;StudioCvarStartupCandidate& state;uintptr_t first,second;StudioCvarPendingNodeCandidate* a;StudioCvarPendingNodeCandidate* b;bool mutate;std::vector<unsigned> events;
 unsigned cvarSlot8(uintptr_t p){check(p==first&&services.cvarRegistered&&state.flags==0x800000);events.push_back(1);return 7;}
 void nodeSlot3(StudioCvarPendingNodeCandidate* p,unsigned flags){check(flags==((mutate&&p==b)?9u:0x800000u));events.push_back(p==a?2:4);if(mutate&&p==a){p->next=nullptr;state.flags=9;}}
 void nodeSlot11(StudioCvarPendingNodeCandidate* p){events.push_back(p==a?3:5);}
 void cvarSlot31(uintptr_t p){check(p==first);events.push_back(6);if(mutate)services.cvar=second;}
 void cvarSlot40(uintptr_t p){check(p==(mutate?second:first));events.push_back(7);state.head=b;}
};
int main(){static_assert(sizeof(void*)==8,"x64");unsigned objects[2]{};uintptr_t first=reinterpret_cast<uintptr_t>(&objects[0]),second=reinterpret_cast<uintptr_t>(&objects[1]);check(first>UINT32_MAX&&second>UINT32_MAX);unsigned cases=0;std::ifstream input("outputs/native-cvar-startup.bin",std::ios::binary);
 for(bool present:{false,true})for(bool registered:{false,true})for(bool access:{false,true})for(bool mutate:{false,true}){StudioCvarPendingNodeCandidate b,a{&b};StudioShutdownServicesCandidate services;services.cvar=present?first:0;services.cvarRegistered=registered;StudioCvarStartupCandidate state{0,0,second,&a};Ops ops{services,state,first,second,&a,&b,mutate,{}};studioRegisterCvarServicesCandidate(services,state,0x800000,access?first:0,ops);bool invoked=present&&!registered;check(ops.events==(invoked?std::vector<unsigned>{1,2,3,4,5,6,7}:std::vector<unsigned>{})&&services.cvarRegistered==(registered||present)&&services.cvarHandle==(invoked?7u:0xffffffffu)&&state.access==(invoked?(access?first:second):0)&&state.head==(invoked?nullptr:&a)&&a.next==((invoked&&mutate)?nullptr:&b)); auto tag=[&](uintptr_t p){check(p==0||p==first||p==second);return p==first?1u:p==second?2u:0u;};std::array<unsigned,16> wire{},original{};wire[0]=static_cast<unsigned>(ops.events.size());std::copy(ops.events.begin(),ops.events.end(),wire.begin()+1);wire[9]=unsigned(services.cvarRegistered);wire[10]=services.cvarHandle;wire[11]=tag(state.access);wire[12]=state.flags;wire[13]=state.head?1:0;wire[14]=tag(services.cvar);wire[15]=a.next?1:0;input.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(input)&&wire==original);++cases;}
 check(input.peek()==std::char_traits<char>::eof());std::printf("x64_cvar_startup_registration_cases=%u passed\n",cases);
}


