#pragma once
#include "engine_projection_associate_candidate.h"
#include "engine_shadow_node_remove_candidate.h"
//1010A590: captured payload must remain valid during auxiliary/resource calls.
// Nonnull linked shadow node is required: original does not safely release the
// partial node-allocation failure state. No resource-field clear is invented.
template<class Entry,class Head,class SetHead,class AuxRelease,class AuxRecycle,class Resource,class Beyond>
inline void engineProjectionReleaseCandidate(EngineReceiverAllocationCandidate& receivers,
 EngineShadowNodePoolCandidate& nodes,EngineProjectionAssociationObjectCandidate& object,uint16_t index,
 Entry entry,Head head,SetHead setHead,AuxRelease auxRelease,AuxRecycle auxRecycle,Resource resource,Beyond beyond){
 auto* captured=entry(index);
 if(captured->auxiliary!=0xffff){auxRelease(captured->auxiliary);auxRecycle(captured->auxiliary);captured->auxiliary=0xffff;}
 if(captured->resource!=0xffff)resource(captured->object,captured->resource);
 if(object.head==index)object.head=receivers.pool.links[index].next;
 uint16_t handle=captured->handle;auto* node=entry(index)->node;
 if(head(handle)==node)setHead(handle,node->next);
 engineShadowNodeRemoveCandidate(nodes,node,beyond);entry(index)->node=nullptr;
 engineReceiverUnlinkCandidate(receivers.pool,index);
 receivers.pool.links[index].next=uint16_t(receivers.freeHead);receivers.freeHead=index;
}
