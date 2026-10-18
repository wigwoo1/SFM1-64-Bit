#include "controller_default_registration_insert_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Record{unsigned code;};
struct Adapter{int binding=1,container=1;int* service=nullptr;bool missing=false;Record old{1},later{2};Record* result=&old;unsigned inserts=0;std::vector<unsigned> lines;
 int* registrationService(){return service;}void installDefaultRegistrationService(){service=&binding;}
 void guardService(unsigned kind,unsigned mode,unsigned line){assert(mode==1);assert(kind==((line==215 || line==315)?9u:10u));lines.push_back(line);if(line==317)result=&later;}
 int* getServiceContainer(int* s,unsigned mode){assert(s==service && mode==1);return missing?nullptr:&container;}
 Record* nullRecord(){return nullptr;}Record* insertContainer(int* c,Record* r){assert(c==&container && r->code==7);++inserts;return result;}
};
int main(){static_assert(sizeof(void*)==8);Record record{7};Adapter a;assert(defaultControllerRegistrationInsertCandidate(&record,a)==&a.old);assert((a.lines==std::vector<unsigned>{215,218,315,317}) && a.inserts==1);
 Adapter b;b.service=&b.binding;b.missing=true;assert(!defaultControllerRegistrationInsertCandidate(&record,b) && b.lines.empty() && !b.inserts);
 Adapter c;c.result=nullptr;assert(!defaultControllerRegistrationInsertCandidate(&record,c) && c.inserts==1);
 std::cout<<"x64 default registration insertion: create request, guard order, null container and preserved pointer result passed\n";
}
