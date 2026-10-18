#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "animation_keys.h"
#include "clip_hierarchy.h"
#include "graph_vector_key_sampler.h"
#include <cmath>
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("usage: test_vector_key_sampler session.dmx");auto g=loadGraph(argv[1]);uint64_t layers=0,keys=0,empty=0,curved=0;
 for(auto&log:g.elements){if(g.strings[log.type]!="DmeVector3Log")continue;if(!hierarchyRefs(g,log,"curveinfo",1).empty()){++curved;continue;}
  for(auto ref:hierarchyRefs(g,log,"layers",15)){auto&e=g.elements[ref];KeyTrack track(g,e);if(track.times->values.empty()){++empty;continue;}auto sampler=bindGraphVectorKeySampler(g,static_cast<uint32_t>(&log-g.elements.data()),ref);auto view=bindGraphVectorKeyViewCandidate(g,static_cast<uint32_t>(&log-g.elements.data()),ref);++layers;
   auto verify=[&](size_t i){auto tick=sampler.times[i];auto j=std::upper_bound(sampler.times.begin(),sampler.times.end(),tick)-sampler.times.begin()-1;auto expected=sampler.values[j],actual=sampler.sample(tick);auto viewed=view.sample(tick);if(std::memcmp(actual.data(),viewed.data(),12))throw std::runtime_error("vector view bits differ");for(unsigned c=0;c<3;++c)if(std::isfinite(expected[c])&&(!std::isfinite(actual[c])||std::abs(expected[c]-actual[c])>1e-5f*std::max(1.0f,std::abs(expected[c]))))throw std::runtime_error("stored vector key sample mismatch");++keys;};
   for(int32_t tick:{INT32_MIN,-10000,0,12345,10000,INT32_MAX}){auto a=sampler.sample(tick),b=view.sample(tick);if(std::memcmp(a.data(),b.data(),12))throw std::runtime_error("vector view seek bits differ");}for(size_t i=0;i<sampler.times.size();++i)verify(i);for(size_t i=sampler.times.size();i>0;--i)verify(i-1);
  }
 }std::cout<<"layers="<<layers<<" key_queries="<<keys<<" empty_layers="<<empty<<" curved_logs="<<curved<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
