#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "prepared_session_expression_chain_pass.h"
#include "graph_exact_equal.h"
#include <fstream>
static double median(std::vector<double> values){std::sort(values.begin(),values.end());return values.at(values.size()/2);}
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("fixture and output required");auto base=loadGraph(argv[1]);auto roots=hierarchyRefs(base,base.elements.at(0),"activeClip",1);auto discovery=discoverSessionExpressionChainsCandidate(base,roots.at(0));auto typed=discoverSessionAdditionalPlayPathsCandidate(base,roots.at(0),discovery.chains);auto colors=discoverSessionColorCopyChannelsCandidate(base,roots.at(0));std::set<uint32_t>packSet;for(auto id:colors){ChannelBinding b(base,id);packSet.insert(b.source.element);}std::vector<uint32_t>packs(packSet.begin(),packSet.end());
 SessionRuntime serial(base),parallel(base);PreparedSessionExpressionChainPassCandidate serialPass(serial,discovery.chains,typed,colors,packs),parallelPass(parallel,discovery.chains,typed,colors,packs);
 std::vector<double>serialAll,parallelAll,serialRounds,parallelRounds;size_t compared=0;
 for(unsigned round=0;round<4;++round){std::vector<double>serialTimes,parallelTimes;
  for(unsigned frame=0;frame<25;++frame){int32_t tick=int32_t((frame%5)*10000)-10000;SessionExpressionChainPassResultCandidate a,b;double serialMs=0,parallelMs=0;
   auto runSerial=[&]{auto start=std::chrono::steady_clock::now();a=serialPass.step(tick,nullptr,4,0,0);serialMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();};
   auto runParallel=[&]{auto start=std::chrono::steady_clock::now();b=parallelPass.step(tick,nullptr,4,0,4);parallelMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();};
   if((round+frame)%2){runParallel();runSerial();}else{runSerial();runParallel();}
   if(a.executed!=b.executed||a.changes.attributes!=b.changes.attributes||!graphExactEqualCandidate(serial.graph,parallel.graph))throw std::runtime_error("benchmark full graph/order/journal mismatch");++compared;
   if(frame>=5){serialTimes.push_back(serialMs);parallelTimes.push_back(parallelMs);serialAll.push_back(serialMs);parallelAll.push_back(parallelMs);}
  }serialRounds.push_back(median(serialTimes));parallelRounds.push_back(median(parallelTimes));
 }
 std::ofstream out(argv[2]);if(!out)throw std::runtime_error("cannot open benchmark output");
 auto array=[&](const std::vector<double>&values){out<<'[';for(size_t i=0;i<values.size();++i){if(i)out<<',';out<<values[i];}out<<']';};
 out<<"{\n\"scope\":\"prepared candidate playback; both modes use four scalar workers, no vector workers\",\n\"frames_compared\":"<<compared<<",\n\"timed_frames_per_mode\":"<<serialAll.size()<<",\n\"private_chains\":"<<parallelPass.privateChainJobs()<<",\n\"serial_expression_median_ms\":"<<median(serialAll)<<",\n\"four_expression_workers_median_ms\":"<<median(parallelAll)<<",\n\"serial_round_medians_ms\":";array(serialRounds);out<<",\n\"parallel_round_medians_ms\":";array(parallelRounds);out<<",\n\"serial_samples_ms\":";array(serialAll);out<<",\n\"parallel_samples_ms\":";array(parallelAll);out<<",\n\"validation\":\"full graph, journal and execution order equal after every frame\",\n\"limitations\":\"No rendering, native SFM comparison or whole-application speedup measured. Runtime startup and validation excluded; each round discards five warmup frames; invocation order alternates.\"\n}\n";
 std::cout<<"compared="<<compared<<" timed_per_mode="<<serialAll.size()<<" chains="<<parallelPass.privateChainJobs()<<" serial_expr_median_ms="<<median(serialAll)<<" parallel_expr_median_ms="<<median(parallelAll)<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
