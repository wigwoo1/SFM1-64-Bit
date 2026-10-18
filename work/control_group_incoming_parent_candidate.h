#pragma once
#include <cstdint>
//1008c3b0. Attribute/owner lifetime and original IsA supplied by caller.
template<class Element,class IsGroup>
Element* controlGroupIncomingParentCandidate(bool attributePresent,
 std::uint16_t attributeFlags,Element* owner,std::uint32_t attributeSymbol,
 std::uint32_t requestedSymbol,bool sameOwnerRequired,std::uint32_t ownerIdentity,
 std::uint32_t sourceOwnerIdentity,std::uint32_t restriction,IsGroup isGroup){
 if(!attributePresent||restriction>2)return nullptr;
 if(restriction>=1&&(attributeFlags&0x1000u))return nullptr;
 if(restriction==2&&!(attributeFlags&0x800u))return nullptr;
 if(!owner||!isGroup(owner))return nullptr;
 if(requestedSymbol!=0xffffffffu&&attributeSymbol!=requestedSymbol)return nullptr;
 if(sameOwnerRequired&&ownerIdentity!=sourceOwnerIdentity)return nullptr;
 return owner;
}
//1008cf90: find first candidate accepted by1008c3b0 in manager iterator order.
// Candidate callback receives the element's owner identity context separately.
template<class Element,class First,class Next,class Candidate>
Element* firstIncomingControlGroupCandidate(Element* element,First first,Next next,
 Candidate candidate){
 auto iterator=first(element);
 while(iterator){
  auto* owner=candidate(iterator);
  if(owner)return owner;
  iterator=next(iterator);
 }
 return nullptr;
}
