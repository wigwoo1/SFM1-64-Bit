#pragma once

#include "animation_keys.h"
#include "channel_binding.h"

inline const Attribute* sessionParticleBoolFieldCandidate(const Graph& graph,const Element& element,
                                                           const char* name) {
    const auto* value=namedAttribute(graph,element,name);
    if(value && (value->type!=4 || value->values.size()!=1 || value->values.front().wire.size()!=1))
        throw std::runtime_error("particle Boolean attribute schema");
    return value;
}

inline bool sessionParticleBoolValueCandidate(const Graph& graph,const Element& element,const char* name) {
    const auto* value=sessionParticleBoolFieldCandidate(graph,element,name);
    return value && value->values.front().wire.front()!=0;
}

// CDmeBoolLog holds the most recent key, with the first key used before
// the recorded range. This path admits the uncompressed single base layer
// used by the benchmark's simulation and emission controls.
inline bool sampleSessionParticleBoolCandidate(const Graph& graph,const ChannelBinding& binding,
                                                int32_t tick) {
    const auto& log=graph.elements.at(binding.log);
    if(graph.strings.at(log.type)!="DmeBoolLog" || binding.layers.size()>1 ||
       !hierarchyRefs(graph,log,"curveinfo",1).empty())
        throw std::runtime_error("unsupported particle Boolean log layout");
    if(binding.layers.empty())return false;
    const auto& layer=graph.elements.at(binding.layers.front());
    if(graph.strings.at(layer.type)!="DmeBoolLogLayer")throw std::runtime_error("particle Boolean layer type");
    const KeyTrack track(graph,layer);
    if(track.values->type!=18)throw std::runtime_error("particle Boolean layer values");
    if(track.times->values.empty())
        return sessionParticleBoolValueCandidate(graph,log,"usedefaultvalue") &&
               sessionParticleBoolValueCandidate(graph,log,"defaultvalue");
    const auto& times=track.times->values;
    const auto upper=std::upper_bound(times.begin(),times.end(),tick,
                                     [](int32_t value,const Value& key){return value<wireInt(key);});
    const auto& value=track.values->values.at(upper==times.begin()?0:size_t(upper-times.begin()-1));
    if(value.wire.size()!=1)throw std::runtime_error("particle Boolean key width");
    return value.wire.front()!=0;
}
