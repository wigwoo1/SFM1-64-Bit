#pragma once
#include <optional>
#include <cstdint>
#include <stdexcept>
// Raw byte retained: original visibility getter does not normalize own value.
// This reads retained graph storage, not original bool setter notifications.
template<class Projection>
std::optional<std::uint8_t> controlTreeProjectionBoolCandidate(Projection& p,
 std::size_t index,const char* name){
 if(index>=p.elementHandles.size()||!p.scene.resolve(p.elementHandles[index]))return std::nullopt;
 for(const auto& a:p.graph.elements.at(index).attributes){
  if(p.graph.strings.at(a.name)!=name)continue;
  if(a.type!=4)return std::nullopt;
  if(a.values.size()!=1||a.values[0].wire.size()!=1)throw std::runtime_error("control bool payload");
  return static_cast<std::uint8_t>(a.values[0].wire[0]);
 }
 return std::nullopt;
}
