#pragma once
#include "film_shot_time_candidate.h"
#include <algorithm>
#include <optional>
// Owned subclip groups only; orphan-group native fallback remains unsupported.
inline std::optional<uint32_t> filmClipTrackCandidate(const Graph&g,uint32_t clip){
 const auto&e=g.elements.at(clip);if(g.strings.at(e.type)!="DmeFilmClip")throw std::runtime_error("expected film clip");
 auto groups=hierarchyRefs(g,e,"subClipTrackGroup",1);if(groups.empty())return {};
 const auto&group=g.elements.at(groups[0]);if(g.strings.at(group.type)!="DmeTrackGroup")throw std::runtime_error("invalid film group type");
 size_t owners=0;for(const auto&other:g.elements){auto refs=hierarchyRefs(g,other,"subClipTrackGroup",1);for(auto ref:refs)if(ref==groups[0])++owners;}if(owners!=1)throw std::runtime_error("ambiguous film group owner");
 const auto*tracks=namedAttribute(g,group,"tracks");if(!tracks||tracks->type!=15)throw std::runtime_error("invalid film tracks");if(tracks->values.empty())return {};
 auto first=tracks->values[0].reference;if(first==UINT32_MAX)return {};if(first>=g.elements.size())throw std::runtime_error("unresolved first film track");if(g.strings.at(g.elements[first].type)!="DmeTrack")return {};return first;
}
// Reconstructed FindFilmClipAtTime method policy, not viewport call-path parity.
inline std::optional<uint32_t> filmTrackClipAtTimeCandidate(const Graph&g,uint32_t track,int32_t tick){
 const auto&e=g.elements.at(track);if(g.strings.at(e.type)!="DmeTrack")throw std::runtime_error("expected film track");
 const auto*kind=namedAttribute(g,e,"clipType");if(!kind||kind->type!=2||kind->values.size()!=1)throw std::runtime_error("invalid track clip type");
 if(wireInt(kind->values[0])!=3)return {};
 struct Clip{uint32_t id;int32_t start,duration;};std::vector<Clip>clips;
 for(auto id:hierarchyRefs(g,e,"children",15)){const auto&child=g.elements.at(id);if(g.strings.at(child.type)!="DmeFilmClip")continue;auto refs=hierarchyRefs(g,child,"timeFrame",1);if(refs.size()!=1)throw std::runtime_error("film time frame missing");AnimationTimeFrame f(g,g.elements.at(refs[0]));clips.push_back({id,f.start,f.duration});}
 std::sort(clips.begin(),clips.end(),[](const Clip&a,const Clip&b){return a.start<b.start;});
 for(size_t i=1;i<clips.size();++i)if(clips[i-1].start==clips[i].start)throw std::runtime_error("equal-start native clip ordering unverified");
 for(const auto&clip:clips){int64_t end=int64_t(clip.start)+clip.duration;if(end>INT32_MAX||end<INT32_MIN)throw std::runtime_error("film clip end overflow");if(clip.start<=tick&&tick<end)return clip.id;}
 return {};
}
