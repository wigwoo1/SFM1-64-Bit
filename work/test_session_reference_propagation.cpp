#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_reference_propagation.h"
int main(){try{
 static_assert(sizeof(void*)==8,"x64 required");Graph g;
 g.header="<!-- dmx encoding binary 5 format sfm_session 20 -->";
 auto type=internString(g,"DmeElement"),name=internString(g,"node"),ref=internString(g,"ref");
 for(unsigned i=0;i<3;++i){Element e;e.type=type;e.name=name;e.id[0]=char(i+1);Attribute a;a.name=ref;a.type=1;Value v;v.reference=(i+1)%3;a.values.push_back(v);e.attributes.push_back(a);g.elements.push_back(e);}
 SessionRuntime r(std::move(g));MutationCoordinator m;
 for(auto h:r.elements.graphHandles){auto a=r.attributes.find(h,"ref");m.owners[h].attributeFlags[r.attributes.identity(r.graph,r.elements,a)]=0x401;}
 auto root=r.elements.graphHandles[0];
 if(propagateSessionReferences(r,m,root,3,[](){return true;})||!m.queues.propagated.empty())throw std::runtime_error("suppression failed");
 if(propagateSessionReferences(r,m,root,3,[](){return false;})!=3||m.queues.propagated!=std::vector<uint32_t>{root,r.elements.graphHandles[2],r.elements.graphHandles[1]}||m.owners[root].mask!=3||m.owners[r.elements.graphHandles[2]].mask!=1)throw std::runtime_error("cycle/order/mask mismatch");
 if(propagateSessionReferences(r,m,root,3,[](){return false;}))throw std::runtime_error("repeat propagation changed state");
 m.processPropagatedLive([&](uint32_t h){return r.elements.resolve(r.graph,h)!=nullptr;});
 auto blocked=r.elements.graphHandles[2];auto attr=r.attributes.find(blocked,"ref");auto identity=r.attributes.identity(r.graph,r.elements,attr);m.owners[blocked].attributeFlags[identity]|=0x1000;
 if(propagateSessionReferences(r,m,root,3,[](){return false;})!=1||m.queues.propagated!=std::vector<uint32_t>{root})throw std::runtime_error("propagation barrier failed");
 m.processPropagatedLive([&](uint32_t h){return r.elements.resolve(r.graph,h)!=nullptr;});m.owners[blocked].attributeFlags[identity]&=0xefff;
 r.attributes.retire(attr);
 if(propagateSessionReferences(r,m,root,3,[](){return false;})!=1)throw std::runtime_error("stale attribute traversed");
 r.elements.retire(root);if(propagateSessionReferences(r,m,root,3,[](){return false;}))throw std::runtime_error("stale root traversed");
 std::cout<<"live_reference_propagation_checks=6\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
