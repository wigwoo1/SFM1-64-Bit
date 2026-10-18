#include "engine_receiver_unlink_candidate.h"
#include <vector>
#include <stdexcept>
#include <iostream>
#include <limits>
void check(bool value){if(!value)throw std::runtime_error("receiver unlink mismatch");}
int main(){try{unsigned cases=0;
 for(unsigned length=1;length<=128;++length)for(unsigned removed=0;removed<length;++removed){
  std::vector<EngineReceiverLinkCandidate> links(length);
  for(unsigned i=0;i<length;++i)links[i]={uint16_t(i?i-1:0xffff),uint16_t(i+1<length?i+1:0xffff)};
  EngineReceiverPoolCandidate pool{links.data(),int32_t(length),int32_t(length-1),0,length-1,length};
  engineReceiverUnlinkCandidate(pool,int32_t(removed));check(pool.count==length-1);
  std::vector<unsigned> forward,backward;
  for(uint32_t i=pool.head;i!=0xffff;i=links[i].next){check(i<length&&forward.size()<length);forward.push_back(i);}
  for(uint32_t i=pool.tail;i!=0xffff;i=links[i].previous){check(i<length&&backward.size()<length);backward.push_back(i);}
  std::vector<unsigned> expected;for(unsigned i=0;i<length;++i)if(i!=removed)expected.push_back(i);
  check(forward==expected);check(backward==std::vector<unsigned>(expected.rbegin(),expected.rend()));
  check(links[removed].previous==removed&&links[removed].next==removed);
  auto before=links;auto head=pool.head,tail=pool.tail,count=pool.count;engineReceiverUnlinkCandidate(pool,int32_t(removed));check(pool.head==head&&pool.tail==tail&&pool.count==count);
  for(unsigned i=0;i<length;++i)check(before[i].previous==links[i].previous&&before[i].next==links[i].next);++cases;
 }
 EngineReceiverLinkCandidate node{0xffff,0xffff};
 for(int32_t argument:{-1,std::numeric_limits<int32_t>::min(),1,65536,0x12340000}){
  EngineReceiverPoolCandidate pool{&node,1,0,0,0,1};engineReceiverUnlinkCandidate(pool,argument);check(pool.count==1&&node.previous==0xffff&&node.next==0xffff);
 }
 EngineReceiverPoolCandidate gate{&node,1,-1,0,0,1};engineReceiverUnlinkCandidate(gate,0);check(gate.count==1);
 gate.highwater=0;gate.count=0;engineReceiverUnlinkCandidate(gate,0);check(gate.count==0xffffffffu&&gate.head==0xffff&&gate.tail==0xffff);
 std::cout<<"PASS: "<<cases<<" removal topologies with forward/reverse traversal, repeated-removal invariance, full32-bit rejection/highwater gating and32-bit count wrap. Native parity pending.\n";
 return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
