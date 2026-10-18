#pragma once
#include "engine_receiver_allocate_candidate.h"
#include "engine_receiver_insert_candidate.h"
#include "engine_shadow_node_insert_candidate.h"
struct EngineProjectionAssociationObjectCandidate {uint32_t flags=0;uint16_t head=0xffff;};
struct EngineProjectionAssociationEntryCandidate {
 EngineProjectionAssociationObjectCandidate* object=nullptr;
 EngineShadowPoolNodeCandidate* node=nullptr;
 uint16_t handle=0xffff,resource=0xffff,auxiliary=0xffff,fieldE=0xffff;
};
//10109A60/10109990 logical x64 association. Resource callback must preserve
// captured entry address until initialization finishes, as original does.
template<class GrowEntry,class GrowNode,class Entry,class Head,class SetHead,class Resource,class Beyond,class Warn>
inline uint16_t engineProjectionAssociateCandidate(EngineReceiverAllocationCandidate& receivers,
 EngineShadowNodePoolCandidate& nodes,EngineProjectionAssociationObjectCandidate& object,uint32_t handle,
 GrowEntry growEntry,GrowNode growNode,Entry entry,Head head,SetHead setHead,Resource resource,Beyond beyond,Warn warn){
 uint32_t index=engineReceiverAllocateCandidate(receivers,1,growEntry,[&](bool){warn(0);});
 if(uint16_t(index)==0xffff){warn(1);return 0xffff;}
 auto* captured=entry(uint16_t(index));captured->object=&object;
 engineReceiverInsertCandidate(receivers.pool,object.head,index);object.head=uint16_t(index);
 captured->resource=(object.flags&0x800)?uint16_t(resource(handle)):0xffff;
 captured->handle=uint16_t(handle);captured->auxiliary=captured->fieldE=0xffff;captured->node=nullptr;
 auto* node=engineShadowNodeAllocateCandidate(nodes,1,growNode,[&]{warn(2);});
 if(!node){warn(3);return uint16_t(index);} // Retain prior entry/resource.
 node->index=uint16_t(index);auto* previous=head(uint16_t(handle));
 if(previous)engineShadowNodeInsertCandidate(nodes,previous,node,beyond);
 setHead(uint16_t(handle),node);entry(uint16_t(index))->node=node;
 return uint16_t(index);
}
