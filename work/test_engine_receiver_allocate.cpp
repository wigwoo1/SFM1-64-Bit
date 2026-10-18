#include "engine_receiver_allocate_candidate.h"
#include <vector>
#include <stdexcept>
#include <iostream>
void check(bool v){if(!v)throw std::runtime_error("receiver allocation mismatch");}
int main(){try{
 std::vector<EngineReceiverLinkCandidate> nodes(65535);EngineReceiverAllocationCandidate state{{nodes.data(),65535,-1,0xffff,0xffff,0}};
 unsigned grows=0,warnings=0;
 auto grow=[&](auto&,int n){check(n==1);++grows;};auto warn=[&](bool){++warnings;};
 for(uint32_t i=0;i<65535;++i){check(engineReceiverAllocateCandidate(state,0,grow,warn)==i);check(nodes[i].previous==i&&nodes[i].next==i);}
 check(state.freshCount==65535&&state.pool.highwater==65534&&grows==0&&warnings==0);
 check(engineReceiverAllocateCandidate(state,1,grow,warn)==0xffff);check(grows==1&&warnings==1&&state.freshCount==65535);
 state.freeHead=7;nodes[7].next=3;nodes[3].next=0xffff;
 check(engineReceiverAllocateCandidate(state,0x100,grow,warn)==7);check(nodes[7].previous==7&&nodes[7].next==7);
 check(engineReceiverAllocateCandidate(state,0x101,grow,warn)==3);check(nodes[3].previous==0xffff&&nodes[3].next==0xffff);
 check(state.freeHead==0xffff&&state.freshCount==65535);
 EngineReceiverAllocationCandidate empty;EngineReceiverLinkCandidate replacement;
 auto replace=[&](auto& pool,int){pool.links=&replacement;pool.capacity=1;};
 check(engineReceiverAllocateCandidate(empty,1,replace,warn)==0);check(empty.cachedStorage==&replacement&&empty.freshCount==1);
 EngineReceiverAllocationCandidate width{{nodes.data(),65536,65534}};
 unsigned widthWarnings=0;auto widthWarn=[&](bool kind){check(kind);++widthWarnings;};
 check(engineReceiverAllocateCandidate(width,1,grow,widthWarn)==0xffff);check(widthWarnings==1&&width.pool.highwater==65534&&width.freshCount==0);
 std::cout<<"PASS: all65535 valid fresh indices, exhaustion, free-list order, low-byte initialization, growth storage reload and16-bit sentinel rejection. Native parity pending.\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what();return 1;}}
