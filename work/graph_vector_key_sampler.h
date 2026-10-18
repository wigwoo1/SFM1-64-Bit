#pragma once
#include <cstring>
#include "vector_key_sampler.h"
// Short-lived view: graph attribute/storage must outlive sampling without edits.
struct GraphVectorKeyViewCandidate {
 std::vector<int32_t>times;const Attribute*values;CachedKeySearch search;
 std::array<float,3> value(size_t index)const{std::array<float,3>result;std::memcpy(result.data(),values->values.at(index).wire.data(),12);return result;}
 std::array<float,3>sample(int32_t tick){auto i=search.find(times,tick);if(i<0)return value(0);if(size_t(i)==times.size()-1)return value(times.size()-1);auto a=value(size_t(i)),b=value(size_t(i)+1);float fraction=timeFraction(tick,times[i],times[i+1],false);std::array<float,3>result{};for(unsigned c=0;c<3;++c)result[c]=interpolateFloat(a[c],b[c],fraction);return result;}
};
inline GraphVectorKeyViewCandidate bindGraphVectorKeyViewCandidate(const Graph&g,uint32_t logIndex,uint32_t layerIndex){
 const auto&log=g.elements.at(logIndex);const auto&layer=g.elements.at(layerIndex);
 if(g.strings.at(log.type)!="DmeVector3Log"||g.strings.at(layer.type)!="DmeVector3LogLayer")throw std::runtime_error("expected vector log and layer");
 if(!hierarchyRefs(g,log,"curveinfo",1).empty())throw std::runtime_error("vector curve-info sampling not recovered");
 auto layers=hierarchyRefs(g,log,"layers",15);if(std::find(layers.begin(),layers.end(),layerIndex)==layers.end())throw std::runtime_error("layer is not owned by supplied vector log");
 KeyTrack track(g,layer);if(track.values->type!=24)throw std::runtime_error("expected vector3 value array");
 if(track.times->values.empty()||track.times->values.size()>INT32_MAX)throw std::runtime_error("invalid vector key arrays");
 GraphVectorKeyViewCandidate view;view.values=track.values;view.times.reserve(track.times->values.size());for(const auto&v:track.times->values)view.times.push_back(wireInt(v));for(const auto&v:track.values->values)if(v.wire.size()!=12)throw std::runtime_error("invalid vector wire value");return view;
}
// Requires Graph, KeyTrack, and hierarchyRefs. Owner curve info is checked here.
inline VectorKeySampler bindGraphVectorKeySampler(const Graph&g,uint32_t logIndex,uint32_t layerIndex){
 const auto&log=g.elements.at(logIndex);const auto&layer=g.elements.at(layerIndex);
 if(g.strings.at(log.type)!="DmeVector3Log"||g.strings.at(layer.type)!="DmeVector3LogLayer")throw std::runtime_error("expected vector log and layer");
 if(!hierarchyRefs(g,log,"curveinfo",1).empty())throw std::runtime_error("vector curve-info sampling not recovered");
 auto layers=hierarchyRefs(g,log,"layers",15);if(std::find(layers.begin(),layers.end(),layerIndex)==layers.end())throw std::runtime_error("layer is not owned by supplied vector log");
 KeyTrack track(g,layer);if(track.values->type!=24)throw std::runtime_error("expected vector3 value array");
 std::vector<int32_t> times;std::vector<std::array<float,3>> values;
 for(const auto&v:track.times->values)times.push_back(wireInt(v));
 for(const auto&v:track.values->values){if(v.wire.size()!=12)throw std::runtime_error("invalid vector wire value");std::array<float,3> value;std::memcpy(value.data(),v.wire.data(),12);values.push_back(value);}
 return VectorKeySampler(std::move(times),std::move(values));
}
