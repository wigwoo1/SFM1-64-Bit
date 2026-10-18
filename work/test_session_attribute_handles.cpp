#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_attribute_handles.h"
int main(int argc,char**argv){try{
 if(argc!=2)return 1;auto graph=loadGraph(argv[1]);SessionRuntimeHandles elements(graph);SessionAttributeHandles attributes(graph,elements);size_t checks=0;
 for(uint32_t i=0;i<graph.elements.size();++i)for(auto&a:graph.elements[i].attributes){auto handle=attributes.find(elements.graphHandles[i],graph.strings.at(a.name));if(attributes.resolve(graph,elements,handle)!=&a)throw std::runtime_error("attribute binding mismatch");++checks;}
 size_t releaseChecks=0;bool releaseTested=false;
 for(uint32_t i=0;i<graph.elements.size()&&!releaseTested;++i)for(const auto&a:graph.elements[i].attributes)if(a.type==15&&a.values.size()>1){
  auto saved=a;auto name=graph.strings.at(a.name);auto owner=elements.graphHandles[i];auto handle=attributes.find(owner,name);
  if(!attributes.remove(graph,elements,owner,name,[&](const Value&value,uint32_t liveHandle){
   auto live=attributes.resolve(graph,elements,liveHandle);if(liveHandle!=handle||!live||releaseChecks>=saved.values.size()||value.reference!=saved.values[releaseChecks].reference)throw std::runtime_error("reference release order mismatch");++releaseChecks;
  })||attributes.resolve(graph,elements,handle)||releaseChecks!=saved.values.size())throw std::runtime_error("reference attribute removal mismatch");
  graph.elements[i].attributes.push_back(saved);attributes.bind(graph,elements,owner,name);releaseTested=true;break;
 }
 if(!releaseTested)throw std::runtime_error("no reference removal fixture");
 uint32_t id=0;while(id<graph.elements.size()&&graph.elements[id].attributes.empty())++id;if(id==graph.elements.size())throw std::runtime_error("no attribute fixture");
 auto&element=graph.elements[id];auto saved=element.attributes.front();auto name=graph.strings.at(saved.name);auto owner=elements.graphHandles[id];auto old=attributes.find(owner,name);auto oldIdentity=attributes.identity(graph,elements,old);
 if(!attributes.retire(old)||attributes.retire(old))throw std::runtime_error("attribute retirement mismatch");
 element.attributes.erase(element.attributes.begin());element.attributes.push_back(saved);auto fresh=attributes.bind(graph,elements,owner,name);auto freshIdentity=attributes.identity(graph,elements,fresh);
 if(!freshIdentity||freshIdentity==oldIdentity||attributes.resolveIdentity(graph,elements,oldIdentity)||attributes.resolveIdentity(graph,elements,freshIdentity)!=&element.attributes.back()||attributes.resolveIdentity(graph,elements,1))throw std::runtime_error("attribute identity lifecycle mismatch");
 if(old==fresh||attributes.resolve(graph,elements,old)||attributes.resolve(graph,elements,fresh)!=&element.attributes.back())throw std::runtime_error("recreated attribute accepted stale handle");
 element.attributes.reserve(element.attributes.capacity()+100);
 if(attributes.resolveIdentity(graph,elements,freshIdentity)!=&element.attributes.back()||attributes.identity(graph,elements,fresh)!=freshIdentity||attributes.resolve(graph,elements,fresh)!=&element.attributes.back())throw std::runtime_error("attribute vector growth broke binding");
 auto cycling=fresh;unsigned generationChecks=0;bool retiredSlot=false;
 for(unsigned i=0;i<512;++i){auto previous=cycling;if(!attributes.retire(previous))throw std::runtime_error("generation retirement failed");cycling=attributes.bind(graph,elements,owner,name);
  if(attributes.resolve(graph,elements,previous)||attributes.resolve(graph,elements,old)||attributes.resolve(graph,elements,cycling)!=&element.attributes.back())throw std::runtime_error("generation rollover alias");
  if((previous&0x3fffff)!=(cycling&0x3fffff))retiredSlot=true;++generationChecks;
 }
 if(!retiredSlot)throw std::runtime_error("exhausted generation slot was reused");fresh=cycling;
 if(!elements.retire(owner)||attributes.resolve(graph,elements,fresh))throw std::runtime_error("retired owner accepted");
 elements.bind(graph,id);if(attributes.resolve(graph,elements,fresh))throw std::runtime_error("recreated owner accepted stale attribute");
 std::cout<<"attribute_checks="<<checks<<" reference_release_checks="<<releaseChecks<<" lifecycle_checks=5 generation_checks="<<generationChecks<<" identity_checks=4 exhausted_slot_retired=1\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

