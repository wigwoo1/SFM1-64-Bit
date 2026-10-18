#include "mutation_queues.h"
#include <iostream>
#include <stdexcept>
int main(){try{MutationQueues q;uint8_t state=0;
 q.mark(state,11);q.mark(state,11);if(q.marked!=std::vector<uint32_t>{11}||state!=3)throw std::runtime_error("mark suppression mismatch");
 q.appendMarked(11);q.appendMarked(0xffffffff);q.appendPropagated(7);q.appendPropagated(7);q.appendPrepared(9);q.appendPrepared(0xffffffff);
 if(q.marked!=std::vector<uint32_t>{11,11,0xffffffff}||q.propagated!=std::vector<uint32_t>{7,7}||q.prepared!=std::vector<uint32_t>{9,0xffffffff})throw std::runtime_error("queue append mismatch");
 std::vector<uint16_t> flags{0x4080};clearElementMutation(state,flags);q.mark(state,11);
 if(q.marked!=std::vector<uint32_t>{11,11,0xffffffff,11}||state!=3||flags[0]!=0x4000)throw std::runtime_error("reset requeue mismatch");
 q.marked={11,0xffffffff,12};std::vector<uint32_t> events;
 q.processMarked([](uint32_t h){return h==0xffffffff?uint32_t(0):h;},[&](uint32_t h){events.push_back(h);if(h==11)q.appendMarked(13);},[&](uint32_t h){events.push_back(h+100);});
 if(events!=std::vector<uint32_t>{11,111,12,112}||!q.marked.empty()||q.propagated!=std::vector<uint32_t>{7,7}||q.prepared!=std::vector<uint32_t>{9,0xffffffff})throw std::runtime_error("marked drain mismatch");
 std::cout<<"mutation_queue_checks=4 marked_drain_checks=1\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
