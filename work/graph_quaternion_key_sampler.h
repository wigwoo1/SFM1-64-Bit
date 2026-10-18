#pragma once
#include <cstring>
#include "quaternion_key_sampler.h"
// Requires Graph, KeyTrack, and hierarchyRefs. Owner curve info is checked here.
inline QuaternionKeySampler bindGraphQuaternionKeySampler(const Graph&g,uint32_t logIndex,uint32_t layerIndex){
 const auto&log=g.elements.at(logIndex);const auto&layer=g.elements.at(layerIndex);
 if(g.strings.at(log.type)!="DmeQuaternionLog"||g.strings.at(layer.type)!="DmeQuaternionLogLayer")throw std::runtime_error("expected quaternion log and layer");
 if(!hierarchyRefs(g,log,"curveinfo",1).empty())throw std::runtime_error("quaternion curve-info sampling not recovered");
 auto layers=hierarchyRefs(g,log,"layers",15);if(std::find(layers.begin(),layers.end(),layerIndex)==layers.end())throw std::runtime_error("layer is not owned by supplied quaternion log");
 KeyTrack track(g,layer);if(track.values->type!=27)throw std::runtime_error("expected quaternion3 value array");
 std::vector<int32_t> times;std::vector<std::array<float,4>> values;
 for(const auto&v:track.times->values)times.push_back(wireInt(v));
 for(const auto&v:track.values->values){if(v.wire.size()!=16)throw std::runtime_error("invalid quaternion wire value");std::array<float,4> value;std::memcpy(value.data(),v.wire.data(),16);values.push_back(value);}
 return QuaternionKeySampler(std::move(times),std::move(values));
}
