#include "controller_default_registration_lookup_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Adapter{int serviceValue=1,containerValue=2,recordValue=3,replacement=4;int* service=&serviceValue;int* container=&containerValue;int* result=&recordValue;std::vector<unsigned> guards;unsigned lookups=0;
 int* registrationService(){return service;}void installDefaultRegistrationService(){service=&serviceValue;}
 void guardService(unsigned action,unsigned kind,unsigned line){assert(kind==1);assert(line==(action==9?215u:action==10?218u:action==5?298u:300u));guards.push_back(action);if(action==6)result=&replacement;}
 int* getServiceContainer(int* p,unsigned mode){assert(p==service && !mode);return container;}int* nullRecord(){return nullptr;}
 int* lookupContainer(int* p,const unsigned* key){assert(p==container && *key==0x12345000 && guards.back()==5);++lookups;return result;}
};
int main(){unsigned key=0x12345000;Adapter a;assert(defaultControllerRegistrationLookupCandidate(&key,a)==&a.recordValue && (a.guards==std::vector<unsigned>{5,6}) && a.result==&a.replacement);
 Adapter b;b.service=nullptr;assert(defaultControllerRegistrationLookupCandidate(&key,b)==&b.recordValue && (b.guards==std::vector<unsigned>{9,10,5,6}));
 Adapter c;c.container=nullptr;assert(!defaultControllerRegistrationLookupCandidate(&key,c) && c.guards.empty() && !c.lookups);
 std::cout<<"x64 default registration lookup: table binding, container gate, guard order and saved result passed\n";
}
