#include <vector>
#include <iostream>
#include <stdexcept>
#include "studio_decal_association_release_candidate.h"
static void check(bool v){if(!v)throw std::runtime_error("association release contract");}
int main(){try{unsigned transitions=0;
for(unsigned length=1;length<=256;++length){std::vector<StudioDecalAssociationNodeCandidate> nodes(length);std::vector<bool> live(length,true);StudioDecalAssociationPoolCandidate pool;pool.head=nodes.data();pool.tail=&nodes.back();pool.count=length;
for(unsigned i=0;i<length;++i){auto& n=nodes[i];n.resource=uintptr_t(0x100000000ULL+i%4);n.payload=uintptr_t(0x200000000ULL+i);n.previous=i?&nodes[i-1]:nullptr;n.next=i+1<length?&nodes[i+1]:nullptr;}
std::vector<unsigned> freeOrder;
for(unsigned key=0;key<4;++key){unsigned calls=0;studioDecalAssociationReleaseCandidate(pool,uintptr_t(0x100000000ULL+key),[&](auto&,auto* node){check(node->resource==uintptr_t(0x100000000ULL+key));++calls;return false;});unsigned removed=0;for(unsigned i=0;i<length;++i)if(i%4==key){check(live[i]);live[i]=false;freeOrder.insert(freeOrder.begin(),i);++removed;}check(calls==removed);
auto* current=pool.head;StudioDecalAssociationNodeCandidate* previous=nullptr;unsigned survivors=0;for(unsigned i=0;i<length;++i)if(live[i]){check(current==&nodes[i]&&current->previous==previous);previous=current;current=current->next;++survivors;}check(!current&&pool.tail==previous&&pool.count==survivors);current=pool.freeHead;for(unsigned i:freeOrder){check(current==&nodes[i]&&current->previous==current);current=current->next;}check(!current);for(unsigned i=0;i<length;++i)check(nodes[i].resource==uintptr_t(0x100000000ULL+i%4)&&nodes[i].payload==uintptr_t(0x200000000ULL+i));++transitions;}
// Releasing an absent key must not touch either list or call the classifier.
auto* freeBefore=pool.freeHead;studioDecalAssociationReleaseCandidate(pool,77,[](auto&,auto*)->bool{throw std::runtime_error("unexpected classify");});check(pool.freeHead==freeBefore&&!pool.head&&!pool.tail&&!pool.count);++transitions;
}
// The original frontier result suppresses active unlink, but still free-pushes.
StudioDecalAssociationNodeCandidate node;node.resource=8;StudioDecalAssociationPoolCandidate pool{&node,&node,nullptr,1};studioDecalAssociationReleaseCandidate(pool,8,[](auto&,auto*){return true;});check(pool.head==&node&&pool.tail==&node&&pool.count==1&&pool.freeHead==&node&&node.next==nullptr);
std::cout<<"association_transitions="<<transitions<<" frontier_skip=1\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
