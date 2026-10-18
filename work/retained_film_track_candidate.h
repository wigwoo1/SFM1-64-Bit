#pragma once
#include "film_track_selection_candidate.h"
enum class FilmTimelineStateCandidate {Gap,SceneLessClip,SceneClip};
struct FilmTimelineSelectionCandidate {FilmTimelineStateCandidate state;std::optional<FilmShotTimeCandidate> shot;};
// Immutable saved timeline index. Rebuild on structural/time-frame edits.
class RetainedFilmTrackCandidate {
 struct Clip{uint32_t id;AnimationTimeFrame frame;};
 AnimationTimeFrame rootFrame_;std::vector<Clip>clips_;
 std::set<uint32_t>sceneClips_;
 static AnimationTimeFrame frame(const Graph&g,uint32_t clip){auto refs=hierarchyRefs(g,g.elements.at(clip),"timeFrame",1);if(refs.size()!=1)throw std::runtime_error("film time frame missing");return AnimationTimeFrame(g,g.elements.at(refs[0]));}
public:
 RetainedFilmTrackCandidate(const Graph&g,uint32_t root):rootFrame_(frame(g,root)){
  if(!std::isfinite(rootFrame_.scale)||rootFrame_.scale<=0)throw std::runtime_error("unsupported root time scale");
  auto track=filmClipTrackCandidate(g,root);if(!track)return;
  const auto&e=g.elements.at(*track);const auto*kind=namedAttribute(g,e,"clipType");if(!kind||kind->type!=2||kind->values.size()!=1)throw std::runtime_error("invalid track clip type");if(wireInt(kind->values[0])!=3)return;
  for(auto id:hierarchyRefs(g,e,"children",15))if(g.strings.at(g.elements.at(id).type)=="DmeFilmClip"){auto f=frame(g,id);auto end=int64_t(f.start)+f.duration;if(end>INT32_MAX||end<INT32_MIN)throw std::runtime_error("film clip end overflow");if(!std::isfinite(f.scale)||f.scale<=0)throw std::runtime_error("unsupported child time scale");clips_.push_back({id,f});}
  std::sort(clips_.begin(),clips_.end(),[](const Clip&a,const Clip&b){return a.frame.start<b.frame.start;});
  for(size_t i=1;i<clips_.size();++i)if(clips_[i-1].frame.start==clips_[i].frame.start)throw std::runtime_error("equal-start native clip ordering unverified");
  for(const auto&clip:clips_){const auto&child=g.elements.at(clip.id);if(!hierarchyRefs(g,child,"camera",1).empty()&&!hierarchyRefs(g,child,"scene",1).empty())sceneClips_.insert(clip.id);}
 }
 size_t clipCount()const{return clips_.size();}
 FilmTimelineSelectionCandidate classify(int32_t tick)const{auto shot=select(tick);return {shot?(sceneClips_.count(shot->clip)?FilmTimelineStateCandidate::SceneClip:FilmTimelineStateCandidate::SceneLessClip):FilmTimelineStateCandidate::Gap,shot};}
 std::optional<FilmShotTimeCandidate>select(int32_t rootParentTick)const{
  auto tick=rootFrame_.childTime(rootParentTick,false);
  for(const auto&clip:clips_){if(clip.frame.start>tick)break;if(tick<int64_t(clip.frame.start)+clip.frame.duration)return FilmShotTimeCandidate{clip.id,clip.frame.childTime(tick,false),tick};}
  return {};
 }
};
