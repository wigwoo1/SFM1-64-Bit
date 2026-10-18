#pragma once
#include "clip_hierarchy.h"
#include "animation_keys.h"
#include "animation_time_frame.h"
#include <cmath>
struct FilmShotTimeCandidate {uint32_t clip;int32_t localTick;int32_t parentTick;};
// Candidate half-open activation policy, not recovered native shot-selection parity.
// Root input is in the root clip's parent timeline, matching channelLocalTime.
inline std::vector<FilmShotTimeCandidate> activeFilmShotsCandidate(const Graph&g,uint32_t root,int32_t tick){
 std::vector<FilmShotTimeCandidate> result;std::set<uint32_t> visiting;
 auto walk=[&](auto&&self,uint32_t id,int32_t parent)->void{
  if(!visiting.insert(id).second)throw std::runtime_error("cyclic film shot hierarchy");
  const auto&e=g.elements.at(id);const auto&type=g.strings.at(e.type);
  auto follow=[&](const char*name,uint8_t kind,int32_t time){for(auto ref:hierarchyRefs(g,e,name,kind))self(self,ref,time);};
  if(type=="DmeTrackGroup")follow("tracks",15,parent);
  else if(type=="DmeTrack")follow("children",15,parent);
  else if(type=="DmeFilmClip"){
   auto frames=hierarchyRefs(g,e,"timeFrame",1);if(frames.size()!=1)throw std::runtime_error("film time frame missing");
   AnimationTimeFrame frame(g,g.elements.at(frames[0]));
   if(frame.duration<0||!std::isfinite(frame.scale)||frame.scale<=0)throw std::runtime_error("unsupported film time frame");
   const int64_t relative=int64_t(parent)-frame.start;
   if(relative>=0&&relative<frame.duration){
    const auto local=frame.childTime(parent,false);auto cameras=hierarchyRefs(g,e,"camera",1);auto scenes=hierarchyRefs(g,e,"scene",1);
    if(!cameras.empty()&&!scenes.empty())result.push_back({id,local,parent});
    follow("subClipTrackGroup",1,local);
   }
  }else throw std::runtime_error("unexpected film shot hierarchy element");
  visiting.erase(id);
 };walk(walk,root,tick);return result;
}
inline FilmShotTimeCandidate selectFilmShotCandidate(const Graph&g,uint32_t root,int32_t tick){
 auto shots=activeFilmShotsCandidate(g,root,tick);
 if(shots.size()!=1)throw std::runtime_error(shots.empty()?"no active renderable shot":"ambiguous active renderable shots");
 return shots.front();
}
