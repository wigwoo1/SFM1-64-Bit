#pragma once
#include <cstdint>
// Recovered104c4300. Graph must be acyclic: original has no visited set.
// Adapters preserve incoming-reference ordering and typed DAG resolution.
// Return is deliberately not an ordinary shortest-path distance.
template<class Node,class OrdinaryParent,class OverrideParent>
int32_t controlLockParentGuardCandidate(const Node* receiver,const Node* candidate,
 const OrdinaryParent& ordinaryParent,const OverrideParent& overrideParent){
 if(!candidate)return -1;
 if(candidate==receiver)return 0;
 const auto ordinary=controlLockParentGuardCandidate(receiver,ordinaryParent(candidate),ordinaryParent,overrideParent);
 const auto overridden=controlLockParentGuardCandidate(receiver,overrideParent(candidate),ordinaryParent,overrideParent);
 if(ordinary<0&&overridden<0)return -1;
 return (ordinary<overridden?ordinary:overridden)+1;
}
