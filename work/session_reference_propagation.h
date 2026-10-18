#pragma once
#include "session_runtime.h"
#include "mutation_coordinator.h"
// Depth-first native incoming-reference traversal without host stack growth.
// Suppression predicate must not mutate handles, graph, ledger or metadata.
template<class Suppressed> size_t propagateSessionReferences(SessionRuntime&r,MutationCoordinator&m,uint32_t root,uint32_t requested,Suppressed suppressed){
 if(requested>3)throw std::runtime_error("unsupported propagation mask");
 std::vector<std::pair<uint32_t,uint32_t>> pending{{root,requested}};size_t changed=0;
 while(!pending.empty()){
  auto entry=pending.back();pending.pop_back();auto handle=entry.first;
  if(!r.elements.resolve(r.graph,handle))continue;
  auto decision=m.propagateOwner(handle,entry.second,suppressed());
  if(!decision.changed)continue;++changed;
  if(!decision.traverseIncoming)continue;
  auto incoming=r.references.incoming.find(handle);if(incoming==r.references.incoming.end())continue;
  for(auto it=incoming->second.rbegin();it!=incoming->second.rend();++it){
   auto identity=r.attributes.identity(r.graph,r.elements,*it);if(!identity)continue;
   auto record=reinterpret_cast<const SessionAttributeRecord*>(r.attributes.table.resolve(*it));
   auto owner=m.owners.find(record->ownerHandle);if(owner==m.owners.end())continue;
   auto flags=owner->second.attributeFlags.find(identity);
   if(flags==owner->second.attributeFlags.end()||(flags->second&0x1000))continue;
   pending.emplace_back(record->ownerHandle,decision.incomingMask);
  }
 }
 return changed;
}
