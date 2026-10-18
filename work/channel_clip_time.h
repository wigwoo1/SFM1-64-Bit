#pragma once
#include "animation_time_frame.h"
// Preserve every hierarchy path; shared channels must not silently use the
// first path. Runtime activation/overlap selection is a separate requirement.
inline std::map<uint32_t,std::vector<std::vector<uint32_t>>> channelTimePaths(const Graph&graph,uint32_t root){
 std::map<uint32_t,std::vector<std::vector<uint32_t>>> paths;std::set<uint32_t> visiting;std::vector<uint32_t> frames;
 auto walk=[&](auto&&self,uint32_t id)->void{
  if(!visiting.insert(id).second)throw std::runtime_error("cyclic channel time hierarchy");const auto&element=graph.elements.at(id);const auto&type=graph.strings.at(element.type);
  auto follow=[&](const char*name,uint8_t kind){for(auto next:hierarchyRefs(graph,element,name,kind))self(self,next);};
  if(type=="DmeChannel")paths[id].push_back(frames);
  else if(type=="DmeTrackGroup")follow("tracks",15);
  else if(type=="DmeTrack")follow("children",15);
  else if(type.find("Clip")!=std::string::npos){auto frame=hierarchyRefs(graph,element,"timeFrame",1);if(frame.size()!=1)throw std::runtime_error("missing channel clip time frame");frames.push_back(frame[0]);follow("trackGroups",15);if(type=="DmeFilmClip")follow("subClipTrackGroup",1);if(type=="DmeChannelsClip")follow("channels",15);frames.pop_back();}
  else throw std::runtime_error("unexpected channel time hierarchy element");visiting.erase(id);
 };walk(walk,root);return paths;
}
inline int32_t channelLocalTime(const Graph&graph,const std::vector<uint32_t>&frames,int32_t rootTick,bool clamp){
 for(auto frame:frames)rootTick=AnimationTimeFrame(graph,graph.elements.at(frame)).childTime(rootTick,clamp);return rootTick;
}
