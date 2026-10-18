#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <random>
#include <iostream>
using AttributeIdentity=std::pair<uint32_t,std::string>;
struct AttributeOperation{uint32_t id;std::vector<AttributeIdentity>inputs,outputs;};
#include "declared_access_layers_candidate.h"
int main(){try{
 const AttributeIdentity a{1,"x"},b{2,"x"};
 std::vector<AttributeOperation>operations{{0,{a},{}},{1,{a},{}},{2,{}, {a}},{3,{a},{b}},{4,{}, {b}}};
 if(declaredAccessLayersCandidate(operations,{0,1,2,3,4})!=std::vector<std::vector<uint32_t>>{{0,1},{2},{3},{4}})throw std::runtime_error("hazard layers mismatch");
 std::mt19937 rng(5073);for(size_t trial=0;trial<1000;++trial){operations.clear();std::vector<uint32_t>order;
  for(uint32_t i=0;i<50;++i){AttributeOperation op{i,{},{}};for(unsigned j=0;j<rng()%4;++j)op.inputs.push_back({rng()%8,"value"});for(unsigned j=0;j<rng()%3;++j)op.outputs.push_back({rng()%8,"value"});operations.push_back(op);order.push_back(i);}
  std::shuffle(order.begin(),order.end(),rng);auto layers=declaredAccessLayersCandidate(operations,order);
  std::map<AttributeIdentity,uint64_t>serial,batched;auto run=[&](auto&state,uint32_t id){uint64_t value=id+1;for(const auto&input:operations[id].inputs)value=(value*33+state[input])%1000003;for(const auto&output:operations[id].outputs)state[output]=value;};
  for(auto id:order)run(serial,id);for(const auto&layer:layers){std::set<AttributeIdentity>reads,writes;
   for(auto id:layer){for(const auto&input:operations[id].inputs)if(writes.count(input))throw std::runtime_error("same layer read/write conflict");for(const auto&output:operations[id].outputs)if(writes.count(output)||reads.count(output))throw std::runtime_error("same layer write conflict");reads.insert(operations[id].inputs.begin(),operations[id].inputs.end());writes.insert(operations[id].outputs.begin(),operations[id].outputs.end());}
   for(auto it=layer.rbegin();it!=layer.rend();++it)run(batched,*it);
  }if(serial!=batched)throw std::runtime_error("layer result mismatch");
 }
 bool rejected=false;try{declaredAccessLayersCandidate(operations,{0});}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("incomplete schedule accepted");
 std::cout<<"access_layers_hazards=passed randomized_serial_equivalence=1000 invalid_schedule=passed\n";return 0;
 }catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
