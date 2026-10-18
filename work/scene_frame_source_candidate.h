#pragma once
#include "session_timeline_frames_candidate.h"
#include "scene_tick_sequence_candidate.h"
class SceneFrameSourceCandidate {
 std::vector<int32_t>explicitTicks_;std::optional<SessionTimelineFramesCandidate>timeline_;
public:
 SceneFrameSourceCandidate&operator=(std::vector<int32_t>ticks){timeline_.reset();explicitTicks_=std::move(ticks);return *this;}
 void setTimeline(SessionTimelineFramesCandidate timeline){explicitTicks_.clear();timeline_=timeline;}
 size_t size()const{return timeline_?timeline_->size():explicitTicks_.size();}
 bool empty()const{return size()==0;}
 int32_t at(size_t frame)const{return timeline_?timeline_->tick(frame):explicitTicks_.at(frame);}
 int32_t front()const{return at(0);}
 int32_t operator[](size_t frame)const{return at(frame);}
 bool automatic()const{return bool(timeline_);}
};
inline bool sceneFrameSourceMatchesInitialTickCandidate(const SceneFrameSourceCandidate& frames,
                                                         size_t initialFrame, int32_t expectedTick) {
 return initialFrame < frames.size() && frames.at(initialFrame) == expectedTick;
}
