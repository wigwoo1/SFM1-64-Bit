#include "controller_registration_startup_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Adapter{
 std::uint32_t first[2]={7,8},second[2]={9,10};bool registered=false;std::vector<std::uint32_t> tags;std::vector<void*> tables;unsigned lookups=0;
 void invokeRegistrationHelper(std::uint32_t tag){tags.push_back(tag);}
 std::uint32_t firstTableCode(){return first[0];}bool lookupRegistration(std::uint32_t code){assert(code==7 && tags.size()%26==0);++lookups;return registered;}
 auto firstTable(){return first;}auto secondTable(){return second;}
 void registerTable(std::uint32_t mode,void* table){assert(mode==0);tables.push_back(table);registered=true;}
};
int main(){static_assert(sizeof(void*)==8);Adapter a;registerControllerStartupTablesCandidate(a);assert(a.tags.size()%26==0 && a.tags.front()==0x10476b10 && a.tags.back()==0x10476560 && a.tables.size()==2 && a.tables[0]==a.first && a.tables[1]==a.second && a.lookups==1);registerControllerStartupTablesCandidate(a);assert(a.tags.size()==52 && a.tables.size()==2);
 std::cout<<"x64 registration startup: 26-helper order, guard lookup and two native table pointers passed\n";
}
