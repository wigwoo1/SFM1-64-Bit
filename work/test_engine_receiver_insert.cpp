#include "engine_receiver_insert_candidate.h"
#include "engine_receiver_allocate_candidate.h"
#include <vector>
#include <iostream>
#include <stdexcept>
#include <algorithm>
void check(bool v){if(!v)throw std::runtime_error("receiver insertion mismatch");}
void verify(const EngineReceiverPoolCandidate& pool,const std::vector<unsigned>& expected){
 std::vector<unsigned> forward,backward;
 for(uint32_t i=pool.head;i!=0xffff;i=pool.links[i].next){check(forward.size()<expected.size());forward.push_back(i);}
 for(uint32_t i=pool.tail;i!=0xffff;i=pool.links[i].previous){check(backward.size()<expected.size());backward.push_back(i);}
 check(forward==expected);check(backward==std::vector<unsigned>(expected.rbegin(),expected.rend()));check(pool.count==expected.size());
}
int main(){try{unsigned cases=0;
 for(unsigned length=1;length<=64;++length)for(unsigned position=0;position<=length;++position){
  std::vector<EngineReceiverLinkCandidate> links(length+1);
  EngineReceiverAllocationCandidate state{{links.data(),int32_t(length+1),-1}};
  auto grow=[](auto&,int){throw std::runtime_error("unexpected growth");};auto warn=[](bool){throw std::runtime_error("unexpected warning");};
  std::vector<unsigned> expected;
  for(unsigned i=0;i<length;++i){check(engineReceiverAllocateCandidate(state,0,grow,warn)==i);engineReceiverInsertCandidate(state.pool,0xffff,i);expected.push_back(i);}
  check(engineReceiverAllocateCandidate(state,0,grow,warn)==length);
  engineReceiverInsertCandidate(state.pool,position<length?position:0xffff,length);expected.insert(expected.begin()+position,length);verify(state.pool,expected);
  // Moving an already linked node must unlink before reinserting at tail.
  engineReceiverInsertCandidate(state.pool,0xffff,length);expected.erase(expected.begin()+position);expected.push_back(length);verify(state.pool,expected);
  engineReceiverUnlinkCandidate(state.pool,int32_t(length));expected.pop_back();verify(state.pool,expected);++cases;
 }
 // Actual association allocation passes1 and starts withffff links.
 // Insertion first invokesunlink: native count32 wraps then increments.
 EngineReceiverLinkCandidate associationNode{0xffff,0xffff};
 EngineReceiverPoolCandidate associationPool{&associationNode,1,0,0xffff,0xffff,0};
 engineReceiverInsertCandidate(associationPool,0xffff,0);
 check(associationPool.head==0&&associationPool.tail==0&&associationPool.count==0);
 check(associationNode.previous==0xffff&&associationNode.next==0xffff);
 engineReceiverUnlinkCandidate(associationPool,0);
 check(associationPool.head==0xffff&&associationPool.tail==0xffff&&associationPool.count==0xffffffffu);
 std::cout<<"PASS: "<<cases<<" allocate/insert/move/unlink lifecycles with ordered forward/reverse traversal and counts. Initialized-ffff path count wrap checked; native parity pending.\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what();return 1;}}
