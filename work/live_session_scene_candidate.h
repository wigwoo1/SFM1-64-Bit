#pragma once
#include "retained_shot_timeline_candidate.h"
#include "session_render_scene_candidate.h"
struct LiveSessionSceneStepCandidate {RetainedShotStepCandidate evaluation;SessionRenderSceneCandidate scene;};
inline LiveSessionSceneStepCandidate evaluateLiveShutterSampleCandidate(RetainedShotTimelineCandidate&timeline,int32_t outputTick,int32_t sampleTick,uint32_t width,uint32_t height,size_t scalarWorkers=2,size_t vectorWorkers=2,size_t expressionWorkers=0,const SessionCameraSamplePointsCandidate* cameraSamples=nullptr){
 auto evaluation=timeline.stepAtShutterSample(outputTick,sampleTick,scalarWorkers,vectorWorkers,expressionWorkers);
 auto scene=readSessionRenderSceneCandidate(timeline.graph(),evaluation.shot.clip,width,height,cameraSamples);
 return {std::move(evaluation),std::move(scene)};
}
inline LiveSessionSceneStepCandidate evaluateLiveSessionSceneCandidate(RetainedShotTimelineCandidate&timeline,int32_t tick,uint32_t width,uint32_t height,size_t scalarWorkers=2,size_t vectorWorkers=2,size_t expressionWorkers=0){
 return evaluateLiveShutterSampleCandidate(timeline,tick,tick,width,height,scalarWorkers,vectorWorkers,expressionWorkers);
}
