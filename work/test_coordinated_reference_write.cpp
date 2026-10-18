#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "coordinated_reference_write.h"
#include "attribute_initial_flags.h"
#include <iostream>
int main(int argc,char**argv){try{if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;
 for(uint32_t i=0;i<r.graph.elements.size();++i)for(const auto&a:r.graph.elements[i].attributes)if(a.type==1&&a.values[0].reference!=0xfffffffe){auto owner=r.elements.graphHandles[i];auto name=r.graph.strings[a.name];auto handle=r.attributes.find(owner,name);auto identity=r.attributes.identity(r.graph,r.elements,handle);auto old=a.values[0].reference;uint32_t id=old==0?1:0;auto target=r.elements.graphHandles[id];
  auto&o=m.owners[owner];o.attributeFlags[identity]=initialAttributeFlags(a.type)|0x2000;o.callbacks.waiting={{7,identity}};m.cycle.phase=3;
  if(coordinatedReferenceWrite(r,m,owner,name,0,target,false)||a.values[0].reference!=old||!m.queues.marked.empty()||!m.queues.prepared.empty()||o.attributeFlags[identity]!=0x2401)throw std::runtime_error("rejected phase mutated session");
  m.cycle.phase=4;if(coordinatedReferenceWrite(r,m,owner,name,0,target,false)||a.values[0].reference!=old||!m.queues.marked.empty())throw std::runtime_error("constructor reference restriction ignored");
  m.cycle.phase=2;if(!coordinatedReferenceWrite(r,m,owner,name,0,target,false)||a.values[0].reference!=id||r.references.occurrences(target,handle)==0||m.queues.marked!=std::vector<uint32_t>{owner}||m.queues.prepared!=std::vector<uint32_t>{owner})throw std::runtime_error("coordinated write mismatch");
  if(coordinatedReferenceWrite(r,m,owner,name,0,target,false)||m.queues.marked.size()!=1||m.queues.prepared.size()!=1)throw std::runtime_error("unchanged write queued");
  m.cycle.phase=5;unsigned notified=0,dispatched=0;m.processMarked([&](MutationOwner*live){if(live!=&o||a.values[0].reference!=id)throw std::runtime_error("owner notification mismatch");++notified;});
  m.processPrepared([](uintptr_t,uintptr_t,uint32_t){},[&](uintptr_t callback,uintptr_t attr){if(callback!=7||attr!=identity||r.attributes.resolveIdentity(r.graph,r.elements,attr)!=&a)throw std::runtime_error("live attribute dispatch mismatch");++dispatched;});
  if(notified!=1||dispatched!=1||o.attributeFlags[identity]!=0x6401||o.state!=2)throw std::runtime_error("coordinated processing mismatch");
  std::cout<<"coordinated_session_checks=5 element="<<i<<'\n';return 0;
 }throw std::runtime_error("scalar reference fixture missing");
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
