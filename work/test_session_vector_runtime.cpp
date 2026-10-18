#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_vector_runtime.h"
int main(int argc,char**argv){try{if(argc!=3)return 1;auto g=loadGraph(argv[1]);unsigned channels=0,writes=0;
for(uint32_t id=0;id<g.elements.size();++id){if(g.strings.at(g.elements[id].type)!="DmeChannel")continue;ChannelBinding b(g,id);if(!b.destination.present()||b.destination.scalarType!=10||b.log==0xffffffff||b.layers.size()!=1)continue;
const auto&log=g.elements.at(b.log);if(g.strings.at(log.type)!="DmeVector3Log"||!hierarchyRefs(g,log,"curveinfo",1).empty())continue;KeyTrack track(g,g.elements.at(b.layers[0]));if(track.times->values.empty())continue;
auto sampler=bindGraphVectorKeySampler(g,b.log,b.layers[0]);b.mode=3;++channels;
for(auto tick:{sampler.times.front(),sampler.times.back(),sampler.times.front()+(sampler.times.back()-sampler.times.front())/2}){auto expected=sampler.sample(tick);auto*a=runtimeAttribute(g,b.destination.element,b.destination.attribute);for(unsigned c=0;c<3;++c)setFloat(*a,b.destination.index,12345.f+c,c);SessionAttributeChanges changes;
if(!executeSessionVectorPlay(g,b,tick,&changes)||changes.attributes.size()!=1)throw std::runtime_error("vector play write rejected");if(std::memcmp(a->values.at(b.destination.index).wire.data(),expected.data(),12))throw std::runtime_error("vector destination mismatch");if(runtimeWriteVector(g,b.destination,expected,&changes))throw std::runtime_error("unchanged vector write notified");++writes;}
}
if(!channels)throw std::runtime_error("no vector channels exercised");saveGraph(g,argv[2]);auto reload=loadGraph(argv[2]);unsigned replay=0;for(uint32_t id=0;id<reload.elements.size();++id){if(reload.strings.at(reload.elements[id].type)!="DmeChannel")continue;ChannelBinding b(reload,id);b.mode=3;if(executeSessionVectorPlay(reload,b,0))++replay;}
if(replay!=channels)throw std::runtime_error("reload playback count mismatch");std::cout<<"vector_channels="<<channels<<" writes="<<writes<<" reload_plays="<<replay<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
