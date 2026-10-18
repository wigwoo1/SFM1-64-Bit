#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_expression_runtime.h"
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("usage: test_session_empty_float_play session.dmx");auto graph=loadGraph(argv[1]);auto roots=hierarchyRefs(graph,graph.elements[0],"activeClip",1);ClipHierarchy hierarchy(graph,roots.at(0));unsigned count=0,copies=0,defaultsCount=0,forced=0;
 for(auto id:hierarchy.channels){ChannelBinding binding(graph,id);if(binding.mode!=3||!binding.destination.present()||binding.destination.scalarType!=3||binding.log==0xffffffff||binding.layers.size()!=1)continue;
  const auto&log=graph.elements[binding.log];if(graph.strings[log.type]!="DmeFloatLog"||!hierarchyRefs(graph,log,"curveinfo",1).empty())continue;KeyTrack track(graph,graph.elements[binding.layers[0]]);if(!track.times->values.empty())continue;
  FloatLogDefault defaults(graph,log);bool copy=!defaults.enabled&&binding.source.present();if(copy&&binding.source.scalarType!=3)continue;
  float expected=copy?wireFloat(runtimeAttribute(graph,binding.source.element,binding.source.attribute)->values.at(binding.source.index)):defaults.emptyLayerValue();
  if(!executeSessionFloatPlay(graph,binding,0)||wireFloat(runtimeAttribute(graph,binding.destination.element,binding.destination.attribute)->values.at(binding.destination.index))!=expected)throw std::runtime_error("empty playback mismatch");++count;if(copy)++copies;else++defaultsCount;
  if(!executeSessionFloatPlay(graph,binding,0,nullptr,true)||wireFloat(runtimeAttribute(graph,binding.destination.element,binding.destination.attribute)->values.at(binding.destination.index))!=defaults.emptyLayerValue())throw std::runtime_error("forced empty playback mismatch");++forced;
 }
 if(!count)throw std::runtime_error("no empty float playback cases");std::cout<<"empty_play="<<count<<" source_fallback="<<copies<<" owner_defaults="<<defaultsCount<<" forced_checks="<<forced<<'\n';return 0;
 }catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
