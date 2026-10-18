#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_expression_runtime.h"
#include "runtime_channel_copy.h"
int main(int argc,char**argv){try{if(argc!=2)return 1;auto g=loadGraph(argv[1]);SessionRuntimeHandles runtime(g);
 for(uint32_t id=0;id<g.elements.size();++id){if(g.strings[g.elements[id].type]!="DmeChannel")continue;RuntimeFloatCopyBinding b(g,id,runtime);
  auto&copy=b.saved;if(copy.mode!=1||!copy.source.present()||!copy.destination.present()||copy.source.scalarType!=3||copy.destination.scalarType!=3||copy.source.element==copy.destination.element)continue;
  bool arrayChecked=false;for(uint32_t element=0;element<g.elements.size()&&!arrayChecked;++element)for(auto&attribute:g.elements[element].attributes)if(attribute.type==17&&attribute.values.size()>=2){
   auto same=copy;same.source={element,g.strings[attribute.name],0,3};same.destination={element,g.strings[attribute.name],1,3};setFloat(attribute,0,12.5f);setFloat(attribute,1,87.5f);SessionAttributeChanges selfEvents;
   if(!executeSessionFloatCopy(g,same,&selfEvents)||!selfEvents.attributes.empty()||wireFloat(attribute.values[1])!=87.5f)throw std::runtime_error("same array attribute copy was not skipped");arrayChecked=true;break;}
  if(!arrayChecked)throw std::runtime_error("missing array copy fixture");
  for(float value:{0.25f,0.75f,-12.5f}){setFloat(*runtimeAttribute(g,copy.source.element,copy.source.attribute),copy.source.index,value);setFloat(*runtimeAttribute(g,copy.destination.element,copy.destination.attribute),copy.destination.index,12345);SessionAttributeChanges changes;
   if(!executeRuntimeFloatCopy(g,runtime,b,&changes)||changes.attributes.size()!=1||wireFloat(runtimeAttribute(g,copy.destination.element,copy.destination.attribute)->values[copy.destination.index])!=value)throw std::runtime_error("live copy mismatch");}
  auto*mode=runtimeAttribute(g,id,"mode");setWireU32(mode->values[0],0);SessionAttributeChanges edits;
  if(executeRuntimeFloatCopy(g,runtime,b,&edits)||!edits.attributes.empty())throw std::runtime_error("mode edit kept stale binding");setWireU32(mode->values[0],1);
  auto*toElement=runtimeAttribute(g,id,"toElement");auto*toName=runtimeAttribute(g,id,"toAttribute");auto oldRef=toElement->values[0].reference;auto oldName=toName->values[0].text;
  toElement->values[0].reference=copy.source.element;toName->values[0].text=copy.source.attribute;
  if(executeRuntimeFloatCopy(g,runtime,b,&edits)||!edits.attributes.empty())throw std::runtime_error("endpoint edit kept stale binding");
  toElement->values[0].reference=oldRef;toName->values[0].text=oldName;
  runtime.retire(b.destinationHandle);setFloat(*runtimeAttribute(g,copy.destination.element,copy.destination.attribute),copy.destination.index,54321);
  if(executeRuntimeFloatCopy(g,runtime,b))throw std::runtime_error("retired destination accepted");
  runtime.bind(g,copy.destination.element);if(executeRuntimeFloatCopy(g,runtime,b)||wireFloat(runtimeAttribute(g,copy.destination.element,copy.destination.attribute)->values[copy.destination.index])!=54321)throw std::runtime_error("stale destination alias");
  RuntimeFloatCopyBinding refreshed(g,id,runtime);if(!executeRuntimeFloatCopy(g,runtime,refreshed))throw std::runtime_error("refreshed binding rejected");
  runtime.retire(refreshed.sourceHandle);if(executeRuntimeFloatCopy(g,runtime,refreshed))throw std::runtime_error("retired source accepted");
  std::cout<<"runtime_float_copy_checks=10 channel="<<id<<'\n';return 0;
 }throw std::runtime_error("no usable float copy");
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
