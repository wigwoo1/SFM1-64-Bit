#include "controller_registration_lookup_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Service{int marker;};struct Record{unsigned code;std::uintptr_t payload;};
struct Adapter{
 Service fallback{1},external{2};Service* service=nullptr;Record record{0,0x100000001ULL};bool match=true,inject=false;unsigned installs=0,key=0;std::vector<unsigned> guards;
 Service* registrationService(){return service;}void guardService(unsigned action,unsigned kind,unsigned line){assert(kind==1 && line==(action==9?215u:218u));guards.push_back(action);if(action==9 && inject)service=&external;}
 void installDefaultRegistrationService(){++installs;service=&fallback;}
 Record* lookupServiceSlot2(Service* p,const unsigned* input){assert(p==service);key=*input;return match?&record:nullptr;}
 std::uintptr_t recordPayload(Record* p){return p->payload;}std::uintptr_t nullPayload(){return 0;}
};
int main(){static_assert(sizeof(void*)==8);Adapter a;assert(lookupControllerRegistrationCandidate(0x12345678,a)==0x100000001ULL && a.key==0x12345000 && a.installs==1 && (a.guards==std::vector<unsigned>{9,10}));lookupControllerRegistrationCandidate(0xffffffff,a);assert(a.key==0xfffff000 && a.guards.size()==2);
 Adapter b;b.inject=true;b.match=false;assert(!lookupControllerRegistrationCandidate(0,b) && !b.installs && b.service==&b.external);
 std::cout<<"x64 registration lookup: key mask, payload above4GiB, default install, live guard binding and missing record passed\n";
}
