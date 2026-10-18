#include "engine_shadow_node_remove_candidate.h"
#include <vector>
#include <iostream>
#include <stdexcept>
void check(bool v){if(!v)throw std::runtime_error("node removal mismatch");}
int main(){try{unsigned cases=0;
 for(unsigned length=1;length<=64;++length)for(unsigned removed=0;removed<length;++removed){
  std::vector<EngineShadowPoolNodeCandidate> nodes(length);
  for(unsigned i=0;i<length;++i){nodes[i].index=uint16_t(i);nodes[i].previous=i?&nodes[i-1]:nullptr;nodes[i].next=i+1<length?&nodes[i+1]:nullptr;}
  EngineShadowNodePoolCandidate pool;pool.head=&nodes.front();pool.tail=&nodes.back();pool.linkedCount=length;
  unsigned predicates=0;engineShadowNodeRemoveCandidate(pool,&nodes[removed],[&](auto*){++predicates;return false;});
  check(predicates==1&&pool.linkedCount==length-1&&pool.freeHead==&nodes[removed]&&!nodes[removed].next);
  unsigned visited=0;auto* prior=static_cast<EngineShadowPoolNodeCandidate*>(nullptr);
  for(auto* node=pool.head;node;node=node->next){check(node!=&nodes[removed]&&node->previous==prior&&visited<length);prior=node;++visited;}
  check(visited==length-1&&prior==pool.tail);
  auto grow=[](auto&,int){};auto warn=[](){};check(engineShadowNodeAllocateCandidate(pool,1,grow,warn)==&nodes[removed]);check(!pool.freeHead&&!nodes[removed].previous&&!nodes[removed].next);++cases;
 }
 EngineShadowPoolNodeCandidate detached;detached.previous=&detached;detached.next=&detached;EngineShadowNodePoolCandidate pool;engineShadowNodeRemoveCandidate(pool,&detached,[](auto*){return false;});check(pool.linkedCount==0&&pool.freeHead==&detached&&!detached.next);
 std::cout<<"PASS: "<<cases<<" linked removal/reallocation topologies and detached-node free behavior. Native parity pending.\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what();return 1;}}
