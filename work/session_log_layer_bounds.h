#pragma once
#include "animation_keys.h"
#include "log_layer_selection.h"
// Infinite flags are runtime fields (native +0x64/+0x65), not inferred
// from saved key bounds. Caller supplies explicit runtime state per layer.
inline std::vector<LogLayerBounds> sessionLogBounds(const Graph&graph,const std::vector<uint32_t>&layers,const std::vector<std::pair<bool,bool>>&infinite){
 if(layers.size()!=infinite.size())throw std::runtime_error("missing runtime infinite flags");std::vector<LogLayerBounds> bounds;
 for(size_t i=0;i<layers.size();++i){KeyTrack track(graph,graph.elements.at(layers[i]));LogLayerBounds layer{infinite[i].first,infinite[i].second,{}};for(const auto&value:track.times->values)layer.times.push_back(wireInt(value));bounds.push_back(std::move(layer));}return bounds;
}
