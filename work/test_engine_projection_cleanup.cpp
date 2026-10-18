#include "engine_projection_cleanup_candidate.h"
#include "engine_projection_release_candidate.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
void check(bool v){if(!v)std::abort();}
int main(){unsigned total=0;
 for(unsigned length=0;length<=64;++length){std::vector<EngineReceiverLinkCandidate> links(length);std::vector<EngineProjectionAssociationEntryCandidate> entries(length);EngineReceiverAllocationCandidate receivers;receivers.pool={links.data(),int32_t(length),-1,0xffff,0xffff,0};std::vector<EngineShadowPoolNodeCandidate> storage(length);EngineShadowNodeBlockCandidate block{nullptr,int32_t(length),storage.data()};EngineShadowNodePoolCandidate nodes;nodes.first=&block;EngineProjectionAssociationObjectCandidate objects[3];for(auto& o:objects)o.flags=0x800;EngineShadowPoolNodeCandidate* heads[4]{};std::vector<bool> removed(length);unsigned released=0;
  for(unsigned i=0;i<length;++i)check(engineProjectionAssociateCandidate(receivers,nodes,objects[i%3],i%4,[](auto&,int){check(false);},[](auto&,int){check(false);},[&](uint16_t n){return &entries[n];},[&](uint16_t h){return heads[h];},[&](uint16_t h,auto* n){heads[h]=n;},[&](uint32_t){return i;},[](auto*){return false;},[](int){check(false);})==i);
  unsigned order[4]={1,3,0,2};for(unsigned group:order){
   int previous=int(length);engineProjectionCleanupCandidate(group|0xabcd0000u,[&](uint16_t h){return heads[h];},[&](uint16_t n){return &entries[n];},[&](auto* object,uint16_t n){check(n<previous&&n%4==group&&object==&objects[n%3]);previous=n;
    engineProjectionReleaseCandidate(receivers,nodes,*object,n,[&](uint16_t i){return &entries[i];},[&](uint16_t h){return heads[h];},[&](uint16_t h,auto* node){heads[h]=node;},[](uint16_t){check(false);},[](uint16_t){check(false);},[&](auto* o,uint16_t resource){check(o==object&&resource==n&&!removed[n]);removed[n]=true;++released;},[](auto*){return false;});++total;
   },[&](uint16_t h,EngineShadowPoolNodeCandidate* node){check(h==group&&!node);heads[h]=node;});check(!heads[group]);
   for(unsigned g=0;g<3;++g){uint16_t cursor=objects[g].head;for(int n=int(length)-1;n>=0;--n)if(!removed[n]&&unsigned(n)%3==g){check(cursor==unsigned(n));cursor=links[cursor].next;}check(cursor==0xffff);}
   for(unsigned g=0;g<4;++g){auto* cursor=heads[g];for(int n=int(length)-1;n>=0;--n)if(!removed[n]&&unsigned(n)%4==g){check(cursor==&storage[n]);cursor=cursor->next;}check(!cursor);}
  }
  check(released==length);std::vector<bool> reused(length);for(unsigned i=0;i<length;++i){auto* node=engineShadowNodeAllocateCandidate(nodes,0,[](auto&,int){check(false);},[]{check(false);});check(node>=storage.data()&&node<storage.data()+length);unsigned index=unsigned(node-storage.data());check(!reused[index]);reused[index]=true;}check(!nodes.freeHead);
 }
 std::printf("PASS: %u cleanup releases across lengths0..64, three objects/four shadows, captured successor traversal and all node reuse. Native lifecycle parity pending.\n",total);
}
