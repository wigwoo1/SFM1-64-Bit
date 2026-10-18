#include "native_container_composition_fixture.h"
#include "controller_table_registration_candidate.h"
#include "controller_default_registration_insert_candidate.h"
#include "controller_default_registration_lookup_candidate.h"
struct ServiceAdapter:Adapter{
 int serviceTag=1;int* service=nullptr;unsigned creates=0;std::vector<unsigned> guardLines;
 int* registrationService(){return service;}void installDefaultRegistrationService(){service=&serviceTag;}
 void guardService(unsigned,unsigned,unsigned line){guardLines.push_back(line);}
 Adapter* getServiceContainer(int* s,unsigned mode){assert(s==&serviceTag);creates+=mode;return this;}
 Item* nullRecord(){return nullptr;}
 Item* insertContainer(Adapter* c,Item* r){return insertControllerContainerCandidate(r,*c);}
 Item* registerServiceSlot3(int*,Item* r){return defaultControllerRegistrationInsertCandidate(r,*this);}
 Item* lookupContainer(Adapter* c,const unsigned* key){return lookupControllerContainerCandidate(key,*c);}
};
int main(){static_assert(sizeof(void*)==8);ServiceAdapter a;std::vector<Item> first(1025),second(1025);
 for(unsigned i=0;i<1024;++i){first[i].code=(i+1)<<12;second[i].code=first[i].code;}
 registerControllerTableCandidate(0x12,first.data(),a);assert(a.count==1024 && a.inserted==1024 && a.creates==1024);a.validate();
 for(unsigned i=0;i<1024;++i){assert(first[i].code==(0x12000000u|((i+1)<<12)));assert(defaultControllerRegistrationLookupCandidate(&first[i].code,a)==&first[i]);}
 registerControllerTableCandidate(0x12,second.data(),a);assert(a.count==1024 && a.replaced==1024);a.validate();
 for(unsigned i=0;i<1024;++i)assert(defaultControllerRegistrationLookupCandidate(&first[i].code,a)==&second[i]);
 assert(a.guardLines[0]==215 && a.guardLines[1]==218 && !a.error);
 std::cout<<"native x64 table/service/container composition: 1024 registrations, mode-tagged keys, replacements and native pointer lookups passed\n";
}
