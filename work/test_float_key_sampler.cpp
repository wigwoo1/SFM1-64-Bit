#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "animation_keys.h"
#include "clip_hierarchy.h"
#include "float_key_sampler.h"
#include <cmath>
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("usage: test_float_key_sampler session.dmx");auto g=loadGraph(argv[1]);uint64_t layers=0,keys=0,empty=0,curved=0;
 for(auto&log:g.elements){if(g.strings[log.type]!="DmeFloatLog")continue;if(!hierarchyRefs(g,log,"curveinfo",1).empty()){++curved;continue;}
  for(auto ref:hierarchyRefs(g,log,"layers",15)){auto&e=g.elements[ref];KeyTrack track(g,e);if(track.times->values.empty()){++empty;continue;}FloatKeySampler sampler(g,e);++layers;
   auto verify=[&](size_t i){auto tick=sampler.times[i];auto j=std::upper_bound(sampler.times.begin(),sampler.times.end(),tick)-sampler.times.begin()-1;auto expected=sampler.values[j],actual=sampler.sample(tick);if(std::isfinite(expected)&&(!std::isfinite(actual)||std::abs(expected-actual)>1e-5f*std::max(1.0f,std::abs(expected))))throw std::runtime_error("stored-key sample mismatch");++keys;};
   for(size_t i=0;i<sampler.times.size();++i)verify(i);for(size_t i=sampler.times.size();i>0;--i)verify(i-1);
  }
 }std::cout<<"layers="<<layers<<" key_queries="<<keys<<" empty_layers="<<empty<<" curved_logs="<<curved<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
