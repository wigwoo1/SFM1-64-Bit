#pragma once
#include <set>
// Conservative admission for sampling against a stable pre-commit graph.
// Includes source even when current defaults choose a different playback path.
inline std::set<AttributeIdentity> scalarSamplingReadsCandidate(const Graph&graph,uint32_t channel,const ChannelBinding&binding){
 std::set<AttributeIdentity>reads;
 auto element=[&](uint32_t id){for(const auto&attribute:graph.elements.at(id).attributes)reads.insert({id,graph.strings.at(attribute.name)});};
 element(channel);if(binding.source.present())reads.insert({binding.source.element,binding.source.attribute});
 if(binding.log!=0xffffffff){element(binding.log);for(auto info:hierarchyRefs(graph,graph.elements.at(binding.log),"curveinfo",1))element(info);}
 for(auto layer:binding.layers)element(layer);return reads;
}
inline bool scalarSamplingReadSetStableCandidate(uint32_t operation,const std::set<AttributeIdentity>&reads,const std::map<AttributeIdentity,std::vector<uint32_t>>&writers){
 for(const auto&read:reads){auto found=writers.find(read);if(found!=writers.end())for(auto writer:found->second)if(writer!=operation)return false;}return true;
}
