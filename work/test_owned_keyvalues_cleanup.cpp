#include <array>
#include <vector>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include "studio_owned_keyvalues_cleanup_candidate.h"
static void check(bool b){if(!b)std::abort();}
struct Ops {
 std::array<StudioOwnedKeyValuesNodeCandidate,4>& nodes;std::array<unsigned,8>& payloads;std::vector<unsigned> events;unsigned payloadCalls=0;
 void freePayload(uintptr_t p){++payloadCalls;if(!p){events.push_back(0);return;}for(unsigned i=0;i<8;++i)if(p==reinterpret_cast<uintptr_t>(&payloads[i])){check(p>UINT32_MAX);events.push_back(100+i);return;}std::abort();}
 void freeNode(StudioOwnedKeyValuesNodeCandidate* node){for(unsigned i=0;i<4;++i)if(node==&nodes[i]){check(reinterpret_cast<uintptr_t>(node)>UINT32_MAX&&node->payloadA==0&&node->payloadB==0);events.push_back(200+i);return;}std::abort();}
};
struct MutationOps {
 StudioOwnedKeyValuesNodeCandidate& node;uintptr_t first,second;unsigned calls=0,nodeCalls=0;
 void freePayload(uintptr_t p){if(calls++==0){check(p==first&&node.payloadA==first);node.payloadB=second;}else{check(p==second&&node.payloadA==0);node.payloadA=first;node.payloadB=first;}}
 void freeNode(StudioOwnedKeyValuesNodeCandidate*){++nodeCalls;}
};
int main(){static_assert(sizeof(void*)==8,"x64");unsigned cases=0;std::ifstream input("outputs/native-keyvalues-cleanup.bin",std::ios::binary);
 for(unsigned mask:{0u,1u,2u,3u})for(bool selfPeer:{false,true}){
 std::array<StudioOwnedKeyValuesNodeCandidate,4> nodes{};std::array<unsigned,8> payloads{};
 for(unsigned i=0;i<4;++i){nodes[i].payloadA=(mask&1)?reinterpret_cast<uintptr_t>(&payloads[2*i]):0;nodes[i].payloadB=(mask&2)?reinterpret_cast<uintptr_t>(&payloads[2*i+1]):0;}
 nodes[0].child=&nodes[1];nodes[1].peer=&nodes[2];nodes[0].peer=selfPeer?&nodes[0]:&nodes[3];Ops ops{nodes,payloads,{}};studioDeleteOwnedKeyValuesCandidate(&nodes[0],ops);
 std::vector<unsigned> expected;auto add=[&](unsigned i){expected.push_back((mask&1)?100+2*i:0);expected.push_back((mask&2)?101+2*i:0);expected.push_back(200+i);};add(1);add(2);if(!selfPeer)add(3);add(0);check(ops.events==expected&&ops.payloadCalls==(selfPeer?6u:8u));check(nodes[0].child==&nodes[1]&&nodes[0].peer==(selfPeer?&nodes[0]:&nodes[3])&&nodes[1].peer==nullptr&&nodes[2].peer==nullptr);std::array<unsigned,16> wire{},original{};wire[0]=static_cast<unsigned>(ops.events.size());std::copy(ops.events.begin(),ops.events.end(),wire.begin()+1);input.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(input)&&wire==original);auto before=ops.events;studioDeleteOwnedKeyValuesCandidate(nullptr,ops);check(ops.events==before);++cases;
 }
  check(input.peek()==std::char_traits<char>::eof());std::array<unsigned,2> mutationPayloads{};uintptr_t first=reinterpret_cast<uintptr_t>(&mutationPayloads[0]),second=reinterpret_cast<uintptr_t>(&mutationPayloads[1]);StudioOwnedKeyValuesNodeCandidate node{first,0,nullptr,nullptr};MutationOps mutation{node,first,second};studioCleanupOwnedKeyValuesCandidate(node,mutation);check(mutation.calls==2&&mutation.nodeCalls==0&&node.payloadA==first&&node.payloadB==0);
 std::printf("x64_owned_keyvalues_cleanup_order_cases=%u passed\n",cases);
}



