#pragma once
#include <cstdint>
#include <cstring>
#include <cstddef>
// Name association retained from import; payload always read from live storage.
// Imported structure must survive; newly inserted attributes need name registration.
template<class Projection,class Element>
auto controlTreeProjectionAttributeCandidate(Projection& p,Element* owner,
 const char* name,std::uint32_t type)->decltype(p.scene.attributes.resolve(0u)){
 if(!owner)return nullptr;
 for(auto* a=owner->attributes;a;a=a->next){
  auto found=p.attributeLocations.find(a->token);
  if(found==p.attributeLocations.end())continue;
  const auto location=found->second;
  const auto& source=p.graph.elements.at(location.first).attributes.at(location.second);
  if(std::strcmp(p.graph.strings.at(source.name).c_str(),name)!=0)continue;
  return (a->flags&31u)==type?a:nullptr;
 }
 return nullptr;
}
template<class Projection,class Element>
std::size_t controlTreeProjectionReferenceCountCandidate(Projection& p,
 Element* owner,const char* name){
 auto* a=controlTreeProjectionAttributeCandidate(p,owner,name,15);
 return a?a->references().size():0u;
}
template<class Projection,class Element>
auto controlTreeProjectionReferenceAtCandidate(Projection& p,Element* owner,
 const char* name,std::int32_t index)->decltype(p.scene.resolve(0u)){
 auto* a=controlTreeProjectionAttributeCandidate(p,owner,name,15);
 if(!a||index<0||static_cast<std::size_t>(index)>=a->references().size())return nullptr;
 return p.scene.resolve(a->references()[static_cast<std::size_t>(index)]);
}
