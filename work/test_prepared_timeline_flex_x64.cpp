#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "reference_owned_dmx_projection.h"
#include "prepared_session_expression_chain_pass.h"
#include "prepared_owned_global_flex_pass.h"
#include "graph_exact_equal.h"
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=3)throw std::runtime_error("fixture and content root required");auto base=loadGraph(argv[1]);auto root=hierarchyRefs(base,base.elements.at(0),"activeClip",1).at(0);auto discovery=discoverSessionExpressionChainsCandidate(base,root);auto typed=discoverSessionAdditionalPlayPathsCandidate(base,root,discovery.chains);auto colors=discoverSessionColorCopyChannelsCandidate(base,root);std::set<uint32_t>packSet;for(auto id:colors){ChannelBinding b(base,id);packSet.insert(b.source.element);}std::vector<uint32_t>packs(packSet.begin(),packSet.end());auto external=[](const std::string&,auto&)->uint32_t{throw std::runtime_error("external input");};RecoveredOwnedDmxReferenceProjectionCandidate<Graph>p(base,external),q(base,external);PreparedTimelineOperationProfileCandidate profile;unsigned frames=0;double retainedMilliseconds=0,rebuiltMilliseconds=0,timelineMilliseconds=0,flexMilliseconds=0;
 {SessionRuntime retained(p.graph,BorrowedSessionGraphCandidate{}),reference(q.graph,BorrowedSessionGraphCandidate{});PreparedSessionExpressionChainPassCandidate timeline(retained,discovery.chains,typed,colors,packs);PreparedOwnedGlobalFlexPassCandidate<decltype(p)>flex(p,argv[2],&retained.bindingRevision);auto handles=p.elementHandles;
 auto step=[&](int32_t tick){auto start=std::chrono::steady_clock::now();auto a=timeline.step(tick,&profile,
#ifdef PREPARED_SCALAR_WORKERS
PREPARED_SCALAR_WORKERS
#else
0
#endif
,
#ifdef PREPARED_VECTOR_WORKERS
PREPARED_VECTOR_WORKERS
#else
0
#endif
);
#ifdef PREPARED_SCALAR_WORKERS
if(PREPARED_SCALAR_WORKERS&&!timeline.sampledJobs())throw std::runtime_error("no parallel jobs admitted");
#endif

#ifdef PREPARED_VECTOR_WORKERS
if(PREPARED_VECTOR_WORKERS&&!timeline.vectorSampledJobs())throw std::runtime_error("no vector jobs admitted");
#endif
auto timelineEnd=std::chrono::steady_clock::now();auto fa=flex.step();auto flexEnd=std::chrono::steady_clock::now();timelineMilliseconds+=std::chrono::duration<double,std::milli>(timelineEnd-start).count();flexMilliseconds+=std::chrono::duration<double,std::milli>(flexEnd-timelineEnd).count();retainedMilliseconds+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();start=std::chrono::steady_clock::now();auto b=executeSessionExpressionChainPassCandidate(reference,discovery.chains,tick,typed,colors,packs);auto fb=executeSessionFreshFlexPassCandidate(q,argv[2]);rebuiltMilliseconds+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();if(a.executed!=b.executed||a.changes.attributes!=b.changes.attributes||fa.bindings!=fb.bindings||fa.names!=fb.names||fa.changed!=fb.changed||fa.equal!=fb.equal||!graphExactEqualCandidate(p.graph,q.graph)||p.typedAttributeFlags!=q.typedAttributeFlags||p.scene.queues.dirty.handles!=q.scene.queues.dirty.handles||p.scene.queues.cache.handles!=q.scene.queues.cache.handles||p.scene.queues.registration.handles!=q.scene.queues.registration.handles||p.scene.references.frameworkFlags!=q.scene.references.frameworkFlags||p.elementHandles!=handles)throw std::runtime_error("retained timeline/flex graph or state mismatch");++frames;};
 for(int32_t tick:{-10000,0,10000,10000,-10000})step(tick);ChannelBinding first(base,discovery.chains.at(0).play);for(auto*r:{&retained,&reference}){auto*a=runtimeAttribute(r->graph,first.log,"defaultvalue");if(!a)throw std::runtime_error("default missing");setFloat(*a,0,0.25f);}step(0);
 auto before=p.graph;auto flags=p.typedAttributeFlags;auto dirty=p.scene.queues.dirty.handles;retained.invalidateBindings();for(size_t workers:{0u,1u,4u,8u}){bool stale=false;try{timeline.step(0,nullptr,workers,workers);}catch(const std::runtime_error&e){stale=std::string(e.what()).find("invalidated")!=std::string::npos;}if(!stale||!graphExactEqualCandidate(before,p.graph)||flags!=p.typedAttributeFlags||dirty!=p.scene.queues.dirty.handles)throw std::runtime_error("stale timeline workers wrote");}bool failed=false;try{flex.step();}catch(const std::runtime_error&){failed=true;}if(!failed||!graphExactEqualCandidate(before,p.graph)||flags!=p.typedAttributeFlags||dirty!=p.scene.queues.dirty.handles)throw std::runtime_error("invalidated flex wrote");PreparedOwnedGlobalFlexPassCandidate<decltype(p)>rebuilt(p,argv[2],&retained.bindingRevision);auto rebuiltResult=rebuilt.step();if(rebuiltResult.changed||!graphExactEqualCandidate(before,p.graph))throw std::runtime_error("rebuilt flex changed stable frame");rebuilt.invalidate();failed=false;try{rebuilt.step();}catch(const std::runtime_error&){failed=true;}if(!failed||!graphExactEqualCandidate(before,p.graph))throw std::runtime_error("manual flex invalidation wrote");std::cout<<"retained flex preparation_ms="<<flex.preparationMilliseconds()<<'\n';}
 auto exported=p.exportReferences([](uint32_t,const auto&)->std::string{throw std::runtime_error("external export");});if(!graphExactEqualCandidate(exported,p.graph))throw std::runtime_error("retained export");for(auto*projection:{&p,&q}){projection->scene.cleanup({},[](auto&,bool){},[](auto&){});if(projection->scene.size()||projection->scene.attributes.size()||projection->scene.references.nodes.allocationCount())throw std::runtime_error("teardown");}
 if(profile.workerJobs!=profile.workerScalarJobs+profile.workerVectorJobs)throw std::runtime_error("worker profile count mismatch");
 std::cout<<"worker_jobs="<<profile.workerJobs<<" scalar_jobs="<<profile.workerScalarJobs<<" vector_jobs="<<profile.workerVectorJobs<<" worker_preflight_ms="<<profile.workerPreflightMilliseconds<<" worker_batch_ms="<<profile.workerBatchMilliseconds<<" ordered_execution_ms="<<profile.orderedExecutionMilliseconds<<'\n';std::cout<<"play_time_mapping_ms="<<profile.playTimeMappingMilliseconds<<" play_evaluation_ms="<<profile.playEvaluationMilliseconds<<'\n';for(size_t i=0;i<4;++i)std::cout<<"timeline_category="<<i<<" count="<<profile.counts[i]<<" total_ms="<<profile.milliseconds[i]<<'\n';std::cout<<"retained timeline/flex frames="<<frames<<" graph_journal_order_flags_queues_handles_value_edit_invalidation_export_teardown=passed retained_mean_ms="<<retainedMilliseconds/frames<<" timeline_mean_ms="<<timelineMilliseconds/frames<<" flex_mean_ms="<<flexMilliseconds/frames<<" rebuilt_mean_ms="<<rebuiltMilliseconds/frames<<" startup_export_excluded=1 commit_order=serial\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
