#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "prepared_session_expression_chain_pass.h"
#include <fstream>
// Baseline copied from the metadata check before direct reference comparison.
__declspec(noinline) bool legacyMetadata(const Graph&g,const RuntimeCachedFloatPlayBinding&b){
 const auto&e=g.elements.at(b.channelIndex);auto mode=namedAttribute(g,e,"mode");
 if(!mode||mode->type!=2||mode->values.size()!=1||wireInt(mode->values[0])!=b.saved.mode)return false;
 auto endpoint=[&](const char*side,const ChannelEndpoint&saved){std::string prefix=side;auto refs=hierarchyRefs(g,e,(prefix+"Element").c_str(),1);auto name=namedAttribute(g,e,(prefix+"Attribute").c_str());auto index=namedAttribute(g,e,(prefix+"Index").c_str());
  return (refs.empty()?0xffffffff:refs.at(0))==saved.element&&name&&name->type==5&&name->values.size()==1&&name->values[0].text==saved.attribute&&index&&index->type==2&&index->values.size()==1&&wireInt(index->values[0])==saved.index;};
 auto logs=hierarchyRefs(g,e,"log",1);if((logs.empty()?0xffffffff:logs.at(0))!=b.saved.log)return false;
 return endpoint("from",b.saved.source)&&endpoint("to",b.saved.destination)&&hierarchyRefs(g,g.elements.at(b.saved.log),"layers",15)==b.saved.layers;
}
__declspec(noinline) bool directMetadata(const Graph&g,const RuntimeCachedFloatPlayBinding&b){return cachedPlayMetadataCurrent(g,b);}
static double median(std::vector<double>values){std::sort(values.begin(),values.end());return values.at(values.size()/2);}
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("fixture and output required");auto graph=loadGraph(argv[1]);auto root=hierarchyRefs(graph,graph.elements.at(0),"activeClip",1).at(0);auto discovery=discoverSessionExpressionChainsCandidate(graph,root);auto typed=discoverSessionAdditionalPlayPathsCandidate(graph,root,discovery.chains);SessionRuntime runtime(graph);std::set<uint32_t>ids;for(const auto&chain:discovery.chains)ids.insert(chain.play);for(const auto&item:typed)ids.insert(item.first);
 std::vector<RuntimeCachedFloatPlayBinding>bindings;for(auto id:ids)bindings.emplace_back(runtime.graph,id,runtime.elements,runtime.attributes);
 for(const auto&b:bindings)if(legacyMetadata(runtime.graph,b)!=directMetadata(runtime.graph,b))throw std::runtime_error("metadata result mismatch");
 std::vector<double>oldTimes,newTimes;size_t checksum=0;
 auto run=[&](bool direct){auto start=std::chrono::steady_clock::now();size_t accepted=0;for(const auto&b:bindings)accepted+=direct?directMetadata(runtime.graph,b):legacyMetadata(runtime.graph,b);auto end=std::chrono::steady_clock::now();if(accepted!=bindings.size())throw std::runtime_error("unexpected stale metadata");checksum+=accepted;return std::chrono::duration<double,std::milli>(end-start).count();};
 for(unsigned i=0;i<45;++i){double a,b;if(i%2){b=run(true);a=run(false);}else{a=run(false);b=run(true);}if(i>=5){oldTimes.push_back(a);newTimes.push_back(b);}}
 std::ofstream out(argv[2]);if(!out)throw std::runtime_error("cannot write output");out<<"{\n\"scope\":\"paired metadata checks on identical cached play bindings in one process\",\n\"bindings\":"<<bindings.size()<<",\n\"timed_batches_per_mode\":40,\n\"warmup_batches_per_mode\":5,\n\"legacy_median_ms\":"<<median(oldTimes)<<",\n\"direct_median_ms\":"<<median(newTimes)<<",\n\"checksum\":"<<checksum<<",\n";
 auto array=[&](const char*name,const std::vector<double>&values){out<<'\"'<<name<<"\":[";for(size_t i=0;i<values.size();++i){if(i)out<<',';out<<values[i];}out<<']';};array("legacy_samples_ms",oldTimes);out<<",\n";array("direct_samples_ms",newTimes);out<<",\n\"validation\":\"Both implementations agree and accept every fixture binding; invocation order alternates\",\n\"limitations\":\"Valid saved metadata only. This isolates checks, not full playback, rendering or native SFM.\"\n}\n";
 std::cout<<"bindings="<<bindings.size()<<" legacy_median_ms="<<median(oldTimes)<<" direct_median_ms="<<median(newTimes)<<" checksum="<<checksum<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
