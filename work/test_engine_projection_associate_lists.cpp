#include "engine_projection_associate_candidate.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
void check(bool ok){if(!ok)std::abort();}
__declspec(noinline) void unexpected(){check(false);}
int main(){unsigned total=0;
 for(unsigned length=1;length<=64;++length){
  std::vector<EngineReceiverLinkCandidate> links(length);std::vector<EngineProjectionAssociationEntryCandidate> entries(length);EngineReceiverAllocationCandidate receivers;receivers.pool={links.data(),int32_t(length),-1,0xffff,0xffff,0};
  std::vector<EngineShadowPoolNodeCandidate> storage(length);EngineShadowNodeBlockCandidate block{nullptr,int32_t(length),storage.data()};EngineShadowNodePoolCandidate nodes;nodes.first=&block;
  EngineProjectionAssociationObjectCandidate objects[3];for(auto& o:objects)o.flags=0x800;EngineShadowPoolNodeCandidate* heads[4]{};unsigned resources=0;
  for(unsigned i=0;i<length;++i){unsigned group=i%4;auto& object=objects[i%3];
   uint16_t index=engineProjectionAssociateCandidate(receivers,nodes,object,group,[](auto&,int){unexpected();},[](auto&,int){unexpected();},[&](uint16_t n){return &entries[n];},[&](uint16_t h){return heads[h];},[&](uint16_t h,auto* n){heads[h]=n;},[&](uint32_t h){check(h==group);++resources;return i+0x10000;},[](auto*){return false;},[](int){unexpected();});
   check(index==i&&entries[i].object==&object&&entries[i].node==&storage[i]&&entries[i].resource==i&&entries[i].handle==group);++total;
   for(unsigned g=0;g<3;++g){uint16_t cursor=objects[g].head,previous=0xffff;unsigned visited=0;
    for(int n=int(i);n>=0;--n)if(unsigned(n)%3==g){check(cursor==unsigned(n)&&links[cursor].previous==previous);previous=cursor;cursor=links[cursor].next;++visited;}
    check(cursor==0xffff);unsigned reverse=0;for(uint16_t n=previous;n!=0xffff;n=links[n].previous){check(unsigned(n)%3==g);check(++reverse<=visited);}check(reverse==visited);
   }
   for(unsigned g=0;g<4;++g){auto* cursor=heads[g];EngineShadowPoolNodeCandidate* previous=nullptr;unsigned visited=0;
    for(int n=int(i);n>=0;--n)if(unsigned(n)%4==g){check(cursor==&storage[n]&&cursor->index==uint16_t(n)&&cursor->previous==previous);previous=cursor;cursor=cursor->next;++visited;}check(cursor==nullptr);
    unsigned reverse=0;for(auto* n=previous;n;n=n->previous){check(n->index%4==g);check(++reverse<=visited);}check(reverse==visited);
   }
  }
  check(resources==length&&nodes.freshCount==length&&receivers.freshCount==length);
 }
 std::printf("PASS: %u associations across 64 growing-list scenarios, three objects and four shadows; forward/reverse links after every insertion. Native resource/cleanup integration pending.\n",total);
}

