#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_operator_activity.h"
int main(int argc,char**argv){try{if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;SessionMutationDispatch d{r,m};SessionOperatorActivity activity{d};auto handle=r.elements.graphHandles.at(0);auto&o=m.owners[handle];o.state=4;o.attributeFlags[7]=3;
 if(activity.expression(handle))throw std::runtime_error("clean owner active");
 if(!m.begin(handle,7,true)||!activity.expression(handle)||!activity.channel(handle,0,0xffffffff,false,0)||o.state!=7)throw std::runtime_error("mutation activity not shared");
 d.processMarked([](MutationOwner*){});if(o.state!=6||!activity.expression(handle))throw std::runtime_error("marked reset consumed activity");
 if(!activity.prepare(handle)||o.state!=4||activity.expression(handle)||activity.channel(handle,0,0xffffffff,false,0))throw std::runtime_error("prepare shared reset mismatch");
 if(!activity.channel(handle,3,0xffffffff,false,0))throw std::runtime_error("play activity lost");
 auto sourceOwner=r.elements.graphHandles.at(1);auto name=r.graph.strings.at(r.graph.elements[1].attributes.at(0).name);auto sourceHandle=r.attributes.find(sourceOwner,name);auto sourceIdentity=r.attributes.identity(r.graph,r.elements,sourceHandle);auto&sourceMetadata=m.owners[sourceOwner];sourceMetadata.attributeFlags[sourceIdentity]=0x4000;
 if(!activity.channelSource(handle,1,0xffffffff,sourceHandle))throw std::runtime_error("live dirty source inactive");sourceMetadata.attributeFlags[sourceIdentity]=0;if(activity.channelSource(handle,1,0xffffffff,sourceHandle))throw std::runtime_error("clean source active");sourceMetadata.attributeFlags[sourceIdentity]=0x4000;r.attributes.retire(sourceHandle);if(activity.channelSource(handle,1,0xffffffff,sourceHandle))throw std::runtime_error("stale source metadata used");
 auto replacementSource=r.attributes.bind(r.graph,r.elements,sourceOwner,name);auto replacementIdentity=r.attributes.identity(r.graph,r.elements,replacementSource);sourceMetadata.attributeFlags[replacementIdentity]=0x4000;auto cache=sourceHandle;
 if(!activity.channelSourceCached(handle,1,0xffffffff,sourceOwner,name,cache)||cache!=replacementSource)throw std::runtime_error("source fallback did not refresh");
 cache=sourceHandle;if(!activity.channelSourceCached(handle,3,0xffffffff,sourceOwner,name,cache)||cache!=sourceHandle)throw std::runtime_error("play resolved source unnecessarily");
 cache=0xffffffff;if(activity.channelSourceCached(handle,1,0xffffffff,sourceOwner,"missing_attribute",cache)||cache!=0xffffffff)throw std::runtime_error("missing fallback activated");
 m.begin(handle,7,true);r.elements.retire(handle);if(activity.expression(handle)||activity.channel(handle,3,0,false,0)||activity.prepare(handle))throw std::runtime_error("retired owner accepted");
 auto replacement=r.elements.bind(r.graph,0);if(activity.expression(replacement)||activity.prepare(replacement))throw std::runtime_error("replacement inherited metadata");
 std::cout<<"source_fallback_checks=3 source_activity_checks=3 session_operator_activity_checks=6\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
