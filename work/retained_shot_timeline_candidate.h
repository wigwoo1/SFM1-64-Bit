#pragma once
#include "film_shot_time_candidate.h"
#include "retained_film_track_candidate.h"
#include "prepared_session_expression_chain_pass.h"
struct RetainedShotStepCandidate {
 FilmShotTimeCandidate shot;
 size_t executed,changed,scalarJobs,vectorJobs;
 bool rebuilt;
 double operationMilliseconds;
 size_t privateExpressionJobs=0;
};
// Value-only playback retains bindings and worker pools within one shot.
// Shot changes reset to the saved baseline: native cross-shot state is unverified.
class RetainedShotTimelineCandidate {
 Graph baseline;
 uint32_t root;
 uint32_t selected=UINT32_MAX;
 std::unique_ptr<SessionRuntime> runtime;
 std::unique_ptr<PreparedSessionExpressionChainPassCandidate> prepared;
 std::unique_ptr<RetainedFilmTrackCandidate> filmTrack;
public:
 RetainedShotTimelineCandidate(Graph graph,uint32_t rootClip):baseline(std::move(graph)),root(rootClip){if(filmClipTrackCandidate(baseline,root))filmTrack=std::make_unique<RetainedFilmTrackCandidate>(baseline,root);}
 FilmShotTimeCandidate selectShot(int32_t tick)const{
  if(!filmTrack)return selectFilmShotCandidate(baseline,root,tick);
  auto selection=filmTrack->classify(tick);if(selection.state==FilmTimelineStateCandidate::Gap)throw std::runtime_error("no active film clip");
  if(selection.state==FilmTimelineStateCandidate::SceneLessClip)throw std::runtime_error("selected film clip has no renderable scene");
  return *selection.shot;
 }
 const Graph& graph()const{if(!runtime)throw std::runtime_error("shot has not been evaluated");return runtime->graph;}
 const Graph& savedGraph()const{return baseline;}
 uint32_t renderRoot()const{return root;}
 FilmTimelineSelectionCandidate classifyFrame(int32_t tick)const{
  if(filmTrack)return filmTrack->classify(tick);
  auto shots=activeFilmShotsCandidate(baseline,root,tick);if(shots.empty())return {FilmTimelineStateCandidate::Gap,{}};if(shots.size()!=1)throw std::runtime_error("ambiguous active renderable shots");return {FilmTimelineStateCandidate::SceneClip,shots[0]};
 }
 RetainedShotStepCandidate step(int32_t rootParentTick,size_t scalarWorkers=0,size_t vectorWorkers=0,size_t expressionWorkers=0){
  if(scalarWorkers>64||vectorWorkers>64||expressionWorkers>64||scalarWorkers+vectorWorkers+expressionWorkers>64)throw std::runtime_error("combined sampling worker budget exceeds 64");
  auto shot=selectShot(rootParentTick);bool rebuild=selected!=shot.clip||!prepared;
  if(rebuild){
   prepared.reset();runtime.reset();selected=UINT32_MAX;
   auto next=std::make_unique<SessionRuntime>(Graph(baseline));
   auto discovery=discoverSessionExpressionChainsCandidate(next->graph,shot.clip);
   if(!discovery.invalidChannels.empty()||discovery.ambiguousPaths)throw std::runtime_error("unresolved selected-shot discovery");
   auto typed=discoverSessionAdditionalPlayPathsCandidate(next->graph,shot.clip,discovery.chains);
   auto colors=discoverSessionColorCopyChannelsCandidate(next->graph,shot.clip);std::set<uint32_t>packSet;
   for(auto id:colors){ChannelBinding b(next->graph,id);if(next->graph.strings.at(next->graph.elements.at(b.source.element).type)!="DmePackColorOperator")throw std::runtime_error("unsupported selected-shot color source");packSet.insert(b.source.element);}
   std::vector<uint32_t>packs(packSet.begin(),packSet.end());
   auto pass=std::make_unique<PreparedSessionExpressionChainPassCandidate>(*next,discovery.chains,typed,colors,packs);
   runtime=std::move(next);prepared=std::move(pass);selected=shot.clip;
  }
  auto result=prepared->step(shot.parentTick,nullptr,scalarWorkers,vectorWorkers,expressionWorkers);
  return {shot,result.executed.size(),result.changes.attributes.size(),prepared->sampledJobs(),prepared->vectorSampledJobs(),rebuild,result.operationMilliseconds,prepared->privateChainJobs()};
 }
};
