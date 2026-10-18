#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "prepared_session_expression_chain_pass.h"
#include "graph_exact_equal.h"
#include <fstream>
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("fixture and output required");auto base=loadGraph(argv[1]);auto roots=hierarchyRefs(base,base.elements.at(0),"activeClip",1);auto discovery=discoverSessionExpressionChainsCandidate(base,roots.at(0));auto typed=discoverSessionAdditionalPlayPathsCandidate(base,roots.at(0),discovery.chains);auto colors=discoverSessionColorCopyChannelsCandidate(base,roots.at(0));std::set<uint32_t>packSet;for(auto id:colors){ChannelBinding b(base,id);packSet.insert(b.source.element);}std::vector<uint32_t>packs(packSet.begin(),packSet.end());
 SessionRuntime measured(base),reference(base);PreparedSessionExpressionChainPassCandidate pass(measured,discovery.chains,typed,colors,packs),referencePass(reference,discovery.chains,typed,colors,packs);
 PreparedTimelineOperationProfileCandidate profile;double wall=0;unsigned frames=0;
 for(unsigned i=0;i<25;++i){int32_t tick=int32_t(i%5)*10000-10000;auto start=std::chrono::steady_clock::now();auto a=pass.step(tick,i<5?nullptr:&profile,4,0,0);auto end=std::chrono::steady_clock::now();auto b=referencePass.step(tick);
  if(a.executed!=b.executed||a.changes.attributes!=b.changes.attributes||!graphExactEqualCandidate(measured.graph,reference.graph))throw std::runtime_error("profiled playback mismatch");
  if(i>=5){wall+=std::chrono::duration<double,std::milli>(end-start).count();++frames;}
 }
 std::ofstream out(argv[2]);if(!out)throw std::runtime_error("cannot write profile");
 out<<"{\n\"scope\":\"prepared playback, four scalar workers, serial expressions and vectors\",\n\"profiled_frames\":"<<frames<<",\n\"warmup_frames\":5,\n\"mean_instrumented_wall_ms\":"<<wall/frames<<",\n\"mean_worker_preflight_ms\":"<<profile.workerPreflightMilliseconds/frames<<",\n\"mean_worker_batch_ms\":"<<profile.workerBatchMilliseconds/frames<<",\n\"mean_ordered_execution_ms\":"<<profile.orderedExecutionMilliseconds/frames<<",\n\"mean_play_time_mapping_ms\":"<<profile.playTimeMappingMilliseconds/frames<<",\n\"mean_play_evaluation_ms\":"<<profile.playEvaluationMilliseconds/frames<<",\n\"scalar_jobs_per_frame\":"<<profile.workerScalarJobs/frames<<",\n\"categories\":[";
 const char*names[]={"play","expression","pack_color","copy"};for(size_t i=0;i<4;++i){if(i)out<<',';out<<"{\"name\":\""<<names[i]<<"\",\"operations_per_frame\":"<<profile.counts[i]/frames<<",\"mean_ms\":"<<profile.milliseconds[i]/frames<<'}';}
 out<<"],\n\"validation\":\"Full graph, journal and execution order equal against serial prepared playback after all 25 frames\",\n\"limitations\":\"Instrumented times include profiling overhead. Categories are nested within ordered execution; play mapping/evaluation are nested within play. No rendering/native SFM measured.\"\n}\n";
 std::cout<<"profile_frames="<<frames<<" mean_wall_ms="<<wall/frames<<" preflight_ms="<<profile.workerPreflightMilliseconds/frames<<" sampling_ms="<<profile.workerBatchMilliseconds/frames<<" ordered_ms="<<profile.orderedExecutionMilliseconds/frames<<" full_graph_order_journal=pass\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
