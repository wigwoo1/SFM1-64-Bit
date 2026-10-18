#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_runtime.h"
#include "runtime_typed_channel_copy.h"
#include "schedule_traversal.h"
#include "channel_dependencies.h"
int main(int argc,char**argv){try{if(argc!=2)return 1;SessionRuntime owned(loadGraph(argv[1]));auto&g=owned.graph;std::vector<uint32_t>ids;for(uint32_t i=0;i<g.elements.size();++i)if(g.strings[g.elements[i].type]=="DmeChannel")ids.push_back(i);std::map<uint32_t,ChannelBinding>bindings;for(auto id:ids)bindings.emplace(id,ChannelBinding(g,id));unsigned chains=0,queries=0;
for(auto playId:ids){auto play=bindings.at(playId);auto type=play.destination.scalarType;if(!play.destination.present()||(type!=10&&type!=13)||play.log==0xffffffff||play.layers.size()!=1||!hierarchyRefs(g,g.elements[play.log],"curveinfo",1).empty())continue;KeyTrack track(g,g.elements[play.layers[0]]);if(track.times->values.empty())continue;
for(auto copyId:ids){if(copyId==playId)continue;auto copy=bindings.at(copyId);if(!copy.source.present()||!copy.destination.present()||copy.destination.scalarType!=type||!sameRuntimeEndpoint(play.destination,copy.source)||copy.destination.element==copy.source.element&&copy.destination.attribute==copy.source.attribute)continue;
setWireU32(runtimeAttribute(g,playId,"mode")->values[0],3);setWireU32(runtimeAttribute(g,copyId,"mode")->values[0],1);play.mode=3;copy.mode=1;auto schedule=traverseSchedule(channelDependencies({copyId,playId},{copy,play}),{0,1});if(schedule.cycle||schedule.order!=std::vector<uint32_t>{playId,copyId})throw std::runtime_error("typed chain schedule failed");
RuntimeCachedTypedPlayBinding livePlay(g,playId,owned.elements,owned.attributes);RuntimeCachedTypedCopyBinding liveCopy(g,copyId,owned.elements,owned.attributes);
auto first=wireInt(track.times->values.front()),last=wireInt(track.times->values.back());for(auto tick:{first,last,first+(last-first)/2}){unsigned count=type==10?3:4;for(auto endpoint:{play.destination,copy.destination}){auto*a=runtimeAttribute(g,endpoint.element,endpoint.attribute);for(unsigned c=0;c<count;++c)setFloat(*a,endpoint.index,12345.f,c);}
std::vector<char>expected;if(type==10){auto sampler=bindGraphVectorKeySampler(g,play.log,play.layers[0]);auto value=sampler.sample(tick);expected.assign(reinterpret_cast<char*>(value.data()),reinterpret_cast<char*>(value.data())+12);}else{auto sampler=bindGraphQuaternionKeySampler(g,play.log,play.layers[0]);auto value=sampler.sample(tick);expected.assign(reinterpret_cast<char*>(value.data()),reinterpret_cast<char*>(value.data())+16);}
for(auto id:schedule.order){bool ok=id==playId?executeRuntimeTypedPlay(g,owned.elements,owned.attributes,livePlay,tick):executeRuntimeTypedCopy(g,owned.elements,owned.attributes,liveCopy);if(!ok)throw std::runtime_error("typed chain execution failed");}
if(runtimeAttribute(g,copy.destination.element,copy.destination.attribute)->values[copy.destination.index].wire!=expected)throw std::runtime_error("typed chain downstream mismatch");++queries;}++chains;
}}
if(!chains)throw std::runtime_error("no real typed play/copy chains");std::cout<<"typed_play_copy_chains="<<chains<<" downstream_queries="<<queries<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
