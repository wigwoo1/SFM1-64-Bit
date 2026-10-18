#pragma once
#include "control_group_visibility_candidate.h"
#include "control_tree_projection_bool_candidate.h"
#include "control_tree_projection_incoming_owner_candidate.h"
// Original acyclic/non-null group precondition. Pure identity/index callbacks.
// Missing metadata policy belongs to original class registration bridge.
template<class Projection,class Element,class IsGroup,class OwnerIdentity,
 class ElementIndex,class MissingVisible>
std::uint8_t controlTreeProjectionVisibleCandidate(Projection& p,Element* group,
 IsGroup isGroup,OwnerIdentity ownerIdentity,ElementIndex elementIndex,
 MissingVisible missingVisible){
 auto parent=[&](Element* e){return controlTreeProjectionIncomingOwnerCandidate(
  p,e,"children",isGroup,ownerIdentity);};
 auto read=[&](Element* e)->std::uint8_t{
  auto value=controlTreeProjectionBoolCandidate(p,elementIndex(e),"visible");
  return value?*value:missingVisible(e);
 };
 return controlGroupVisibleCandidate(group,parent,read);
}
