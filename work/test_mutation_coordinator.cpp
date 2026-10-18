#include "mutation_coordinator.h"
#include <iostream>
#include <stdexcept>
int main(){try{MutationCoordinator c;auto&o=c.owners[42];uintptr_t a=uintptr_t(0x123456789abcdef0ull);o.attributeFlags[a]=0x2000;o.callbacks.waiting={{7,a}};
 if(!c.begin(42,a,false)||!c.begin(42,a,false)||o.attributeFlags[a]!=0x6080||o.state!=3||c.queues.marked!=std::vector<uint32_t>{42}||c.queues.prepared!=std::vector<uint32_t>{42})throw std::runtime_error("coordinated begin mismatch");
 c.cycle.phase=3;if(c.begin(42,a,false)||o.attributeFlags[a]!=0x6080)throw std::runtime_error("phase rejection changed state");
 c.cycle.phase=5;std::vector<int> events;
 c.processMarked([&](MutationOwner*owner){if(owner!=&o||owner->state!=3||owner->attributeFlags[a]!=0x6080)throw std::runtime_error("mark notification state mismatch");events.push_back(1);});
 if(o.state!=2||o.attributeFlags[a]!=0x6000||!c.queues.marked.empty())throw std::runtime_error("coordinated reset mismatch");
 c.processPrepared([](uintptr_t,uintptr_t,uint32_t){},[&](uintptr_t callback,uintptr_t attribute){if(callback!=7||attribute!=a||!o.callbacks.prepared.empty()||o.callbacks.waiting.empty())throw std::runtime_error("prepared dispatch mismatch");events.push_back(2);});
 if(events!=std::vector<int>{1,2}||!c.queues.prepared.empty())throw std::runtime_error("queue processing order mismatch");
 c.cycle.phase=4;o.attributeFlags[a]|=0x400;if(c.begin(42,a,false))throw std::runtime_error("phase-four restricted mutation accepted");o.attributeFlags[a]&=uint16_t(~0x400);
 if(!c.begin(42,a,true)||!c.queues.prepared.empty()||c.queues.marked!=std::vector<uint32_t>{42})throw std::runtime_error("suppressed preparation mismatch");
 std::cout<<"mutation_coordinator_checks=6\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
