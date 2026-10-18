#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_expression_runtime.h"
#include "runtime_channel_play.h"
int main(int argc,char**argv){try{if(argc!=2)return 1;auto g=loadGraph(argv[1]);SessionRuntimeHandles runtime(g);SessionAttributeHandles attributes(g,runtime);
 for(uint32_t id=0;id<g.elements.size();++id){if(g.strings[g.elements[id].type]!="DmeChannel")continue;RuntimeCachedFloatPlayBinding b(g,id,runtime,attributes);auto&saved=b.saved;
  if(saved.mode!=3||!saved.destination.present()||saved.destination.scalarType!=3||!saved.source.present()||saved.layers.size()!=1||saved.log==0xffffffff||saved.source.element==saved.destination.element||saved.source.element==saved.log)continue;
  if(!hierarchyRefs(g,g.elements[saved.log],"curveinfo",1).empty())continue;KeyTrack track(g,g.elements[saved.layers[0]]);if(track.times->values.empty())continue;
  int32_t tick=wireInt(track.times->values.back());float expected=wireFloat(track.values->values.back());
  auto check=[&](RuntimeCachedFloatPlayBinding&binding){setFloat(*runtimeAttribute(g,saved.destination.element,saved.destination.attribute),saved.destination.index,12345);if(!executeRuntimeFloatPlay(g,runtime,attributes,binding,tick)||wireFloat(runtimeAttribute(g,saved.destination.element,saved.destination.attribute)->values[saved.destination.index])!=expected)throw std::runtime_error("runtime playback mismatch");};
  check(b);
  for(bool source:{true,false}){
   auto endpoint=source?saved.source:saved.destination;auto owner=source?b.sourceHandle:b.destinationHandle;auto old=source?b.sourceAttribute:b.destinationAttribute;
   auto&storage=g.elements.at(endpoint.element).attributes;auto it=std::find_if(storage.begin(),storage.end(),[&](const Attribute&a){return g.strings.at(a.name)==endpoint.attribute;});auto original=*it;
   attributes.retire(old);storage.erase(it);SessionAttributeChanges events;
   if(source)check(b);else if(executeRuntimeFloatPlay(g,runtime,attributes,b,tick,&events)||!events.attributes.empty())throw std::runtime_error("missing destination accepted");
   storage.push_back(original);auto fresh=attributes.bind(g,runtime,owner,endpoint.attribute);check(b);
   if(attributes.resolve(g,runtime,old)||(source?b.sourceAttribute:b.destinationAttribute)!=fresh)throw std::runtime_error("play cache refresh mismatch");
  }
  runtime.retire(b.sourceHandle);check(b);
  std::cout<<"cached_float_play_checks=8 channel="<<id<<'\n';return 0;
 }throw std::runtime_error("no usable keyed float play");
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

