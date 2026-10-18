#pragma once
#include <optional>
#include <cstdint>
#include <stdexcept>
// Nonreference payload remains in retained projection graph. This is not a
// binding to original native wrapper storage or original notification system.
template<class Projection>
std::optional<std::uint32_t> controlTreeProjectionColorCandidate(Projection& p,
 std::size_t elementIndex,const char* name){
 if(elementIndex>=p.elementHandles.size()||!p.scene.resolve(p.elementHandles[elementIndex]))return std::nullopt;
 for(const auto& a:p.graph.elements.at(elementIndex).attributes){
  if(p.graph.strings.at(a.name)!=name)continue;
  if(a.type!=8)return std::nullopt;
  if(a.values.size()!=1||a.values[0].wire.size()!=4)throw std::runtime_error("control color payload");
  std::uint32_t result=0;
  for(unsigned i=0;i<4;++i)result|=std::uint32_t(static_cast<unsigned char>(a.values[0].wire[i]))<<(8*i);
  return result;
 }
 return std::nullopt;
}
