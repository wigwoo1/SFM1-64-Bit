#include "engine_shadow_node_insert_candidate.h"
#include "engine_shadow_node_remove_candidate.h"
#include <vector>
#include <stdexcept>
#include <iostream>
void check(bool v){if(!v)throw std::runtime_error("node insertion mismatch");}
void verify(const EngineShadowNodePoolCandidate& pool,const std::vector<unsigned>& expected){
 std::vector<unsigned> forward,reverse;auto* previous=static_cast<EngineShadowPoolNodeCandidate*>(nullptr);
 for(auto* node=pool.head;node;node=node->next){check(forward.size()<expected.size()&&node->previous==previous);previous=node;forward.push_back(node->index);}
 check(previous==pool.tail);
 for(auto* node=pool.tail;node;node=node->previous){check(reverse.size()<expected.size());reverse.push_back(node->index);}
 check(forward==expected&&reverse==std::vector<unsigned>(expected.rbegin(),expected.rend())&&pool.linkedCount==expected.size());
}
int main(){try{unsigned cases=0;auto beyond=[](auto*){return false;};auto grow=[](auto&,int){};auto warn=[](){};
 for(unsigned length=1;length<=64;++length)for(unsigned position=0;position<=length;++position){
  std::vector<EngineShadowPoolNodeCandidate> nodes(length+1);EngineShadowNodeBlockCandidate block{nullptr,int32_t(length+1),nodes.data()};EngineShadowNodePoolCandidate pool;pool.first=&block;std::vector<unsigned> expected;
  for(unsigned i=0;i<length;++i){auto* node=engineShadowNodeAllocateCandidate(pool,0,grow,warn);check(node==&nodes[i]);node->index=uint16_t(i);engineShadowNodeInsertCandidate(pool,nullptr,node,beyond);expected.push_back(i);}
  auto* inserted=engineShadowNodeAllocateCandidate(pool,0,grow,warn);inserted->index=uint16_t(length);
  engineShadowNodeInsertCandidate(pool,position<length?&nodes[position]:nullptr,inserted,beyond);expected.insert(expected.begin()+position,length);verify(pool,expected);
  engineShadowNodeInsertCandidate(pool,nullptr,inserted,beyond);expected.erase(expected.begin()+position);expected.push_back(length);verify(pool,expected);
  engineShadowNodeRemoveCandidate(pool,inserted,beyond);expected.pop_back();verify(pool,expected);
  auto* reused=engineShadowNodeAllocateCandidate(pool,0,grow,warn);check(reused==inserted);engineShadowNodeInsertCandidate(pool,pool.head,reused,beyond);expected.insert(expected.begin(),length);verify(pool,expected);++cases;
 }
 std::cout<<"PASS: "<<cases<<" node allocate/insert/move/remove/reuse lifecycles; exact forward/reverse order and counts. Frontier predicate/native association path pending.\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what();return 1;}}
