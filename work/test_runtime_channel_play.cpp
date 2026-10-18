#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_expression_runtime.h"
#include "runtime_channel_play.h"
int main(int argc,char**argv){try{if(argc!=2)return 1;auto g=loadGraph(argv[1]);SessionRuntimeHandles runtime(g);
 for(uint32_t id=0;id<g.elements.size();++id){if(g.strings[g.elements[id].type]!="DmeChannel")continue;RuntimeFloatPlayBinding b(g,id,runtime);auto&saved=b.saved;
  if(saved.mode!=3||!saved.destination.present()||saved.destination.scalarType!=3||!saved.source.present()||saved.layers.size()!=1||saved.log==0xffffffff||saved.source.element==saved.destination.element||saved.source.element==saved.log)continue;
  if(!hierarchyRefs(g,g.elements[saved.log],"curveinfo",1).empty())continue;KeyTrack track(g,g.elements[saved.layers[0]]);if(track.times->values.empty())continue;
  int32_t tick=wireInt(track.times->values.back());float expected=wireFloat(track.values->values.back());
  auto check=[&](const RuntimeFloatPlayBinding&binding){setFloat(*runtimeAttribute(g,saved.destination.element,saved.destination.attribute),saved.destination.index,12345);if(!executeRuntimeFloatPlay(g,runtime,binding,tick)||wireFloat(runtimeAttribute(g,saved.destination.element,saved.destination.attribute)->values[saved.destination.index])!=expected)throw std::runtime_error("runtime playback mismatch");};
  check(b);runtime.retire(b.sourceHandle);check(b);
  runtime.retire(b.destinationHandle);if(executeRuntimeFloatPlay(g,runtime,b,tick))throw std::runtime_error("retired destination accepted");
  runtime.bind(g,saved.destination.element);if(executeRuntimeFloatPlay(g,runtime,b,tick))throw std::runtime_error("stale destination accepted");
  RuntimeFloatPlayBinding refreshed(g,id,runtime);check(refreshed);runtime.retire(refreshed.logHandle);if(executeRuntimeFloatPlay(g,runtime,refreshed,tick))throw std::runtime_error("retired log accepted");
  std::cout<<"runtime_float_play_checks=6 channel="<<id<<'\n';return 0;
 }throw std::runtime_error("no usable keyed float play");
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
