#pragma once
#include "film_shot_time_candidate.h"
#include "session_frame_rate_candidate.h"
#include <limits>
// Lazy sampling in the root clip's parent timeline. Rounding and half-open
// endpoint policy are candidates; native timeline/export parity is unverified.
class SessionTimelineFramesCandidate {
 int32_t start_; uint32_t numerator_,denominator_; size_t count_;
public:
 SessionTimelineFramesCandidate(int32_t start,int32_t duration,uint32_t rate):start_(start),numerator_(rate&65535u),denominator_(rate>>16),count_(0){
  if(duration<=0||!numerator_||!denominator_)throw std::runtime_error("invalid timeline duration or rate");
  const int64_t last=int64_t(start)+duration-1;
  if(last>std::numeric_limits<int32_t>::max())throw std::runtime_error("timeline exceeds tick range");
  const uint64_t count=(uint64_t(duration)*numerator_+denominator_-1)/denominator_;
  if(count>std::numeric_limits<size_t>::max())throw std::runtime_error("timeline frame count overflow");
  count_=size_t(count);
 }
 size_t size()const{return count_;}
 int32_t tick(size_t frame)const{
  if(frame>=count_)throw std::runtime_error("timeline frame outside range");
  return int32_t(int64_t(start_)+int64_t(uint64_t(frame)*denominator_/numerator_));
 }
};
inline SessionTimelineFramesCandidate sessionTimelineFramesCandidate(const Graph&graph,uint32_t root){
 const auto&clip=graph.elements.at(root);
 if(graph.strings.at(clip.type)!="DmeFilmClip")throw std::runtime_error("timeline root is not film clip");
 auto refs=hierarchyRefs(graph,clip,"timeFrame",1);
 if(refs.size()!=1)throw std::runtime_error("timeline root time frame missing");
 AnimationTimeFrame frame(graph,graph.elements.at(refs[0]));
 if(!std::isfinite(frame.scale)||frame.scale<=0)throw std::runtime_error("unsupported timeline scale");
 return SessionTimelineFramesCandidate(frame.start,frame.duration,sessionFrameRateCandidate(graph,root,false));
}
