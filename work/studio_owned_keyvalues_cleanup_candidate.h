#pragma once
#include <cstdint>
struct StudioOwnedKeyValuesNodeCandidate {
 std::uintptr_t payloadA=0,payloadB=0;
 StudioOwnedKeyValuesNodeCandidate* peer=nullptr;
 StudioOwnedKeyValuesNodeCandidate* child=nullptr;
};
// Logical pointer-wide graph; original layout and malformed graphs are not supported.
template<class Ops>void studioCleanupOwnedKeyValuesCandidate(StudioOwnedKeyValuesNodeCandidate& node,Ops& ops){
 for(auto* current=node.child;current;){auto* next=current->peer;current->peer=nullptr;studioCleanupOwnedKeyValuesCandidate(*current,ops);ops.freeNode(current);current=next;}
 for(auto* current=node.peer;current&&current!=&node;){auto* next=current->peer;current->peer=nullptr;studioCleanupOwnedKeyValuesCandidate(*current,ops);ops.freeNode(current);current=next;}
 ops.freePayload(node.payloadA);
 auto second=node.payloadB;node.payloadA=0;
 ops.freePayload(second);node.payloadB=0;
}
template<class Ops>void studioDeleteOwnedKeyValuesCandidate(StudioOwnedKeyValuesNodeCandidate* node,Ops& ops){
 if(!node)return;studioCleanupOwnedKeyValuesCandidate(*node,ops);ops.freeNode(node);
}
