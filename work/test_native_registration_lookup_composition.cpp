#include "controller_registration_lookup_candidate.h"
#include "controller_default_registration_lookup_candidate.h"
#include "controller_container_lookup_candidate.h"
#include "controller_container_search_candidate.h"
#include "controller_registration_hash_comparison_candidate.h"
#include <cassert>
#include <array>
#include <string>
#include <iostream>
struct Record{std::uint32_t code;const char* text;};struct Node{Record* data;Node* next;std::uint32_t hash;};
struct Adapter{
 std::array<Record,100> records;std::array<std::string,100> names;std::array<Node,100> nodes;Node* buckets[8]={};
 unsigned calls=0,visits=0,compares=0,hits=0,misses=0,error=99,guards=0;int serviceValue=1;int* service=nullptr;
 Adapter(){for(unsigned i=0;i<100;++i){names[i]="record-"+std::to_string(i);records[i]={(i+1)<<12,names[i].c_str()};auto h=controllerRegistrationHashCandidate(records[i].code);nodes[i]={&records[i],buckets[h%8],h};buckets[h%8]=&nodes[i];}}
 // Bucket population above is fixture setup; insertion is not recovered here.
 unsigned hashKey(const unsigned* key){return controllerRegistrationHashCandidate(*key);}auto compareCallback(){return +[](Record* r,const unsigned* key){return controllerRegistrationCompareCandidate(r->code,*key);};}
 unsigned divisor18(){return 8;}unsigned divisor10(){return 4;}unsigned split14(){return 0;}Node** bucketSlot(unsigned n){return &buckets[n];}
 unsigned hashCalls(){return calls;}void setHashCalls(unsigned n){calls=n;}unsigned visited(){return visits;}void setVisited(unsigned n){visits=n;}unsigned comparisons(){return compares;}void setComparisons(unsigned n){compares=n;}
 void setLookupError(unsigned n){error=n;}unsigned hitCount(){return hits;}void setHitCount(unsigned n){hits=n;}unsigned missCount(){return misses;}void setMissCount(unsigned n){misses=n;}
 Node** findNodeSlot(const unsigned* key,unsigned* output){return searchControllerContainerCandidate(key,output,*this);}Record* nodePayload(Node* n){return n->data;}Record* nullPayload(){return nullptr;}
 int* registrationService(){return service;}void installDefaultRegistrationService(){service=&serviceValue;}void guardService(unsigned,unsigned,unsigned){++guards;}
 Adapter* getServiceContainer(int*,unsigned mode){assert(!mode);return this;}Record* nullRecord(){return nullptr;}Record* lookupContainer(Adapter* p,const unsigned* key){assert(p==this);return lookupControllerContainerCandidate(key,*this);}
 Record* lookupServiceSlot2(int*,const unsigned* key){return defaultControllerRegistrationLookupCandidate(key,*this);}const char* recordPayload(Record* r){return r->text;}
};
// Outer candidate needs its null result to have payload type, so use a facade.
struct Facade:Adapter{const char* nullPayload(){return nullptr;}
 Record* lookupContainer(Adapter* p,const unsigned* key){return p->lookupContainer(p,key);}
};
int main(){static_assert(sizeof(void*)==8);Facade a;
 for(unsigned i=0;i<100;++i){auto* result=lookupControllerRegistrationCandidate(((i+1)<<12)|0xabcu,a);assert(result==a.records[i].text && std::string(result)=="record-"+std::to_string(i));}
 assert(a.hits==100 && !a.misses && a.calls==100 && !a.error && a.visits>100);
 assert(!lookupControllerRegistrationCandidate(0x70000000,a) && a.misses==1 && a.calls==101);
 std::cout<<"native x64 registration composition: 100 records, reason-bit masking, collisions, text pointer identity and missing code passed\n";
}
