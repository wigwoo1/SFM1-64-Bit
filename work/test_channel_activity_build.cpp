#include "registered_operator_build.h"
#include "channel_activity.h"
#include <iostream>
int main(){try{OperatorAttributeRegistration r;r.add(10,{},{});r.add(20,{100},{});std::map<uintptr_t,ChannelActivityState> states;states[10].state42=0x82;states[20].mode=1;uint16_t flags=0x4000;std::vector<uintptr_t> prepared;
 auto build=[&](){return buildRegisteredOperators(r,{0},[&](uintptr_t id){return states.at(id).active(id==20,flags);},[&](uintptr_t id){prepared.push_back(id);states.at(id).prepare();},[&](uintptr_t)->uint16_t&{return flags;},[](uintptr_t,size_t){});};
 auto result=build();if(result.identities!=std::vector<uintptr_t>{20,10}||prepared!=std::vector<uintptr_t>{10,20}||states[10].state42!=0x80||flags)throw std::runtime_error("activity build mismatch");
 prepared.clear();result=build();if(!result.identities.empty()||!prepared.empty())throw std::runtime_error("forced activity repeated");
 flags=0x4000;result=build();if(result.identities!=std::vector<uintptr_t>{20}||prepared!=std::vector<uintptr_t>{20})throw std::runtime_error("dirty reactivation mismatch");
 states[10].mode=3;prepared.clear();result=build();if(result.identities!=std::vector<uintptr_t>{10}||prepared!=std::vector<uintptr_t>{10})throw std::runtime_error("play activity mismatch");
 std::cout<<"channel_activity_build_checks=4\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
