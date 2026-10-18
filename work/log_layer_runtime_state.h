#pragma once
#include <cstdint>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>
// Fresh-layer initialization: 0x10507B40. Copy: 0x1051A9B0.
// This state must follow runtime construction/copy, not serialized key bounds.
struct LogLayerRuntimeState {
 std::map<uint32_t,std::pair<bool,bool>> flags;
 std::map<uint32_t,uint32_t> owners;
 void construct(uint32_t layer){if(!flags.emplace(layer,std::make_pair(false,false)).second)throw std::runtime_error("layer already constructed");owners.emplace(layer,0xffffffff);}
 // 0x104FE550 only sets the owner; copying layer content preserves it.
 void attach(uint32_t layer,uint32_t owner){owners.at(layer)=owner;}
 void setInfinite(uint32_t layer,bool left,bool right){flags.at(layer)={left,right};}
 void copy(uint32_t destination,uint32_t source){flags.at(destination)=flags.at(source);}
 std::vector<std::pair<bool,bool>> forLayers(const std::vector<uint32_t>&layers)const{
  std::vector<std::pair<bool,bool>> result;for(uint32_t layer:layers)result.push_back(flags.at(layer));return result;
 }
};
