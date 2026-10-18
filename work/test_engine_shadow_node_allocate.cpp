#include "engine_shadow_node_allocate_candidate.h"
#include <memory>
#include <vector>
#include <iostream>
#include <stdexcept>
void check(bool v){if(!v)throw std::runtime_error("node pool mismatch");}
struct Block {std::unique_ptr<EngineShadowPoolNodeCandidate[]> nodes;EngineShadowNodeBlockCandidate block;Block(int n):nodes(new EngineShadowPoolNodeCandidate[n]),block{nullptr,n,nodes.get()}{}};
int main(){try{
 std::vector<std::unique_ptr<Block>> blocks;EngineShadowNodePoolCandidate pool;unsigned grows=0,warnings=0;int total=0;
 auto grow=[&](auto& state,int requested){check(requested==1);int count=total?total:3;blocks.push_back(std::make_unique<Block>(count));if(blocks.size()>1)blocks[blocks.size()-2]->block.next=&blocks.back()->block;else state.first=&blocks.back()->block;total+=count;++grows;};
 auto warn=[&](){++warnings;};std::vector<EngineShadowPoolNodeCandidate*> allocated;
 for(unsigned i=0;i<4096;++i){auto* node=engineShadowNodeAllocateCandidate(pool,i%2,grow,warn);check(node);check(node->previous==(i%2?nullptr:node)&&node->next==node->previous);node->index=uint16_t(i);allocated.push_back(node);}
 check(pool.freshCount==4096&&warnings==0&&grows>1);
 for(unsigned i=0;i<allocated.size();++i)check(allocated[i]->index==i);
 pool.freeHead=allocated[7];allocated[7]->next=allocated[3];allocated[3]->next=nullptr;auto frontier=pool.frontier;
 check(engineShadowNodeAllocateCandidate(pool,0x100,grow,warn)==allocated[7]);check(allocated[7]->next==allocated[7]);
 check(engineShadowNodeAllocateCandidate(pool,0x101,grow,warn)==allocated[3]);check(!allocated[3]->next&&pool.freshCount==4096&&pool.frontier.block==frontier.block&&pool.frontier.index==frontier.index);
 EngineShadowNodePoolCandidate failed;failed.field28=99;auto noGrow=[](auto&,int){};check(!engineShadowNodeAllocateCandidate(failed,1,noGrow,warn)&&failed.field28==0&&warnings==1);
 std::cout<<"PASS:4096 allocations across "<<grows<<" blocks, stable node addresses/data, frontier transitions, free-list reuse/low-byte flags and failed growth. Native parity pending.\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what();return 1;}}
