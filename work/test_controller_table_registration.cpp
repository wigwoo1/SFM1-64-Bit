#include "controller_table_registration_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Record{std::uint32_t code;const char* payload;};
struct Adapter{
 int first=1,second=2;int* service=nullptr;unsigned calls=0;bool installAtGuard=false;
 std::vector<unsigned> guards,codes;std::vector<int*> bindings;std::vector<Record*> pointers;
 int* registrationService(){return service;}
 void installDefaultRegistrationService(){service=&first;}
 void guardService(unsigned kind,unsigned mode,unsigned line){assert(mode==1 && line==(kind==9?215u:218u));guards.push_back(kind);if(kind==9 && installAtGuard)service=&second;}
 int registerServiceSlot3(int* binding,Record* r){bindings.push_back(binding);codes.push_back(r->code);pointers.push_back(r);if(++calls==1)service=&second;return -1;}
};
int main(){static_assert(sizeof(void*)==8);Record r[]={{0x12345000,"a"},{0x23456000,"b"},{0,nullptr}};Adapter a;
 registerControllerTableCandidate(0x1ab,r,a);assert((a.guards==std::vector<unsigned>{9,10}) && a.calls==2);assert(r[0].code==0xbb345000 && r[1].code==0xab456000);assert(a.bindings[0]==&a.first && a.bindings[1]==&a.second && a.pointers[1]==r+1);
 registerControllerTableCandidate(0,r,a);assert(a.calls==4 && a.guards.size()==2 && r[0].code==0xbb345000);
 Adapter b;b.installAtGuard=true;Record empty[]={{0,nullptr}};registerControllerTableCandidate(255,empty,b);assert(b.service==&b.second && !b.calls && b.guards.size()==2);
 std::cout<<"x64 table registration: native record pointers, mode OR, sentinel, live service reload and ignored callback failure passed\n";
}
