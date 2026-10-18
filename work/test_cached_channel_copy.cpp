#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_expression_runtime.h"
#include "runtime_channel_copy.h"
int main(int argc,char**argv){try{
 if(argc!=2)return 1;auto graph=loadGraph(argv[1]);SessionRuntimeHandles elements(graph);SessionAttributeHandles attributes(graph,elements);
 for(uint32_t id=0;id<graph.elements.size();++id){if(graph.strings[graph.elements[id].type]!="DmeChannel")continue;
  RuntimeCachedFloatCopyBinding b(graph,id,elements,attributes);auto&copy=b.saved;
  if(copy.mode!=1||!copy.source.present()||!copy.destination.present()||copy.source.scalarType!=3||copy.destination.scalarType!=3||copy.source.element==copy.destination.element)continue;
  auto run=[&](){setFloat(*runtimeAttribute(graph,copy.source.element,copy.source.attribute),copy.source.index,0.625f);setFloat(*runtimeAttribute(graph,copy.destination.element,copy.destination.attribute),copy.destination.index,12345);SessionAttributeChanges events;
   if(!executeRuntimeFloatCopy(graph,elements,attributes,b,&events)||events.attributes.size()!=1||wireFloat(runtimeAttribute(graph,copy.destination.element,copy.destination.attribute)->values.at(copy.destination.index))!=0.625f)throw std::runtime_error("cached copy mismatch");};
  run();
  for(bool source:{true,false}){
   auto owner=source?b.sourceHandle:b.destinationHandle;auto endpoint=source?copy.source:copy.destination;auto old=source?b.sourceAttribute:b.destinationAttribute;
   auto&storage=graph.elements.at(endpoint.element).attributes;auto it=std::find_if(storage.begin(),storage.end(),[&](const Attribute&a){return graph.strings.at(a.name)==endpoint.attribute;});auto saved=*it;
   if(!attributes.retire(old))throw std::runtime_error("retire failed");storage.erase(it);SessionAttributeChanges events;
   if(executeRuntimeFloatCopy(graph,elements,attributes,b,&events)||!events.attributes.empty())throw std::runtime_error("missing attribute accepted");
   storage.push_back(saved);auto fresh=attributes.bind(graph,elements,owner,endpoint.attribute);
   if(attributes.resolve(graph,elements,old)||fresh==old)throw std::runtime_error("attribute generation not advanced");run();
   if((source?b.sourceAttribute:b.destinationAttribute)!=fresh)throw std::runtime_error("expired cache not refreshed");
  }
  std::cout<<"cached_copy_checks=7 channel="<<id<<'\n';return 0;
 }throw std::runtime_error("no usable float copy");
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
