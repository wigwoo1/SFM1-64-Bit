#pragma once
#include <cstring>
#include "control_group_incoming_parent_candidate.h"
// Own runtime token order. Pure callbacks only: token enumeration is a snapshot.
// OwnerIdentity must supply original element owner38; UUID/handle is not a substitute.
template<class Projection,class Element,class IsGroup,class OwnerIdentity>
Element* controlTreeProjectionIncomingOwnerCandidate(Projection& p,Element* target,
 const char* name,IsGroup isGroup,OwnerIdentity ownerIdentity){
 if(!target)return nullptr;
 const auto sourceIdentity=ownerIdentity(target);
 for(auto token:p.scene.references.nodes.tokens(target->record)){
  auto* attribute=p.scene.attributes.resolve(token);
  auto location=p.attributeLocations.find(token);
  if(!attribute||location==p.attributeLocations.end())continue;
  const auto index=location->second.first;
  const auto& source=p.graph.elements.at(index).attributes.at(location->second.second);
  if(std::strcmp(p.graph.strings.at(source.name).c_str(),name)!=0)continue;
  auto* owner=p.scene.resolve(p.elementHandles.at(index));
  // Imported names matched above. Both symbol placeholders represent that name.
  auto* accepted=controlGroupIncomingParentCandidate(true,attribute->flags,owner,
   0u,0u,true,owner?ownerIdentity(owner):0u,sourceIdentity,0u,isGroup);
  if(accepted)return accepted;
 }
 return nullptr;
}
