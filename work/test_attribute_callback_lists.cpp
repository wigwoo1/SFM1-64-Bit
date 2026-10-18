#include "attribute_callback_lists.h"
#include <iostream>
#include <vector>
#include <stdexcept>
int main(){try{static_assert(sizeof(uintptr_t)==8,"x64 required");AttributeCallbackLists lists;
 const uintptr_t attribute=uintptr_t(0x123456789abcdef0ull);lists.waiting={{1,attribute},{2,7},{3,attribute},{4,7},{5,attribute}};lists.prepared={{6,8}};std::vector<uint32_t> queued;
 auto ids=[](const auto&list){std::vector<uintptr_t> result;for(auto item:list)result.push_back(item.callback);return result;};
 auto queue=[&](uint32_t handle){if(ids(lists.waiting)!=std::vector<uintptr_t>{2,4}||ids(lists.prepared)!=std::vector<uintptr_t>{5,3,1,6})throw std::runtime_error("queue ran before list preparation");queued.push_back(handle);};
 lists.prepare(attribute,false,42,queue);
 if(queued!=std::vector<uint32_t>{42})throw std::runtime_error("queue mismatch");
 lists.prepare(attribute,true,43,queue);if(queued.size()!=1)throw std::runtime_error("suppression mismatch");
 lists.prepare(attribute,false,44,queue);if(queued!=std::vector<uint32_t>{42,44})throw std::runtime_error("no-match preparation did not queue");
 if(lists.prepared.front().attribute!=attribute)throw std::runtime_error("64-bit identity truncated");
 lists.waiting={{2,7}};lists.prepared={{3,attribute},{1,attribute}};std::vector<uintptr_t> calls;
 lists.processPrepared([&](uintptr_t callback,uintptr_t value){
  if(value!=attribute)throw std::runtime_error("snapshot attribute mismatch");calls.push_back(callback);
  if(callback==3){if(ids(lists.waiting)!=std::vector<uintptr_t>{3,1,2}||!lists.prepared.empty())throw std::runtime_error("list restoration occurred after callback");
   lists.prepare(7,false,42,[&](uint32_t h){queued.push_back(h);});lists.waiting.push_front({4,8});}
 });
 if(calls!=std::vector<uintptr_t>{3,1}||ids(lists.waiting)!=std::vector<uintptr_t>{4,3,1}||ids(lists.prepared)!=std::vector<uintptr_t>{2})throw std::runtime_error("reentrant preparation changed dispatch snapshot");
 calls.clear();lists.processPrepared([&](uintptr_t callback,uintptr_t value){if(value!=7)throw std::runtime_error("second snapshot mismatch");calls.push_back(callback);});
 if(calls!=std::vector<uintptr_t>{2}||ids(lists.waiting)!=std::vector<uintptr_t>{2,4,3,1}||!lists.prepared.empty())throw std::runtime_error("subsequent dispatch mismatch");
 lists.processPrepared([&](uintptr_t,uintptr_t){throw std::runtime_error("empty dispatch called callback");});
 lists.waiting={{9,7}};lists.prepared={{8,attribute}};std::list<GeneralCallbackRecord> general{{5,1},{5,1},{6,2},{7,4},{10,0}};uint32_t mask=1;std::vector<uintptr_t> order;std::vector<uint32_t> masks;
 lists.process(0x10,mask,attribute,general,[&](uintptr_t callback,uintptr_t owner,uint32_t currentMask){if(owner!=attribute)throw std::runtime_error("general owner truncated");order.push_back(callback);masks.push_back(currentMask);mask=3;if(order.size()==1){general.clear();lists.prepare(7,false,42,[](uint32_t){});}},[&](uintptr_t callback,uintptr_t value){if(value!=attribute)throw std::runtime_error("attribute snapshot replaced");order.push_back(callback);});
 if(order!=std::vector<uintptr_t>{5,5,10,8}||masks!=std::vector<uint32_t>{1,3,3}||ids(lists.prepared)!=std::vector<uintptr_t>{9})throw std::runtime_error("general snapshot dispatch mismatch");
 order.clear();lists.process(0,mask,attribute,std::list<GeneralCallbackRecord>{{1,0}},[&](uintptr_t,uintptr_t,uint32_t){throw std::runtime_error("disabled general dispatch ran");},[&](uintptr_t callback,uintptr_t){order.push_back(callback);});if(order!=std::vector<uintptr_t>{9})throw std::runtime_error("pending attribute dispatch mismatch");
 std::cout<<"callback_list_checks=4 snapshot_dispatch_checks=3 general_dispatch_checks=2 pointer_bits=64\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
