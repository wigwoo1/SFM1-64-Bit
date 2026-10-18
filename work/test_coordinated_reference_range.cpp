#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "coordinated_reference_write.h"
#include "attribute_initial_flags.h"
int main(int argc,char**argv){try{if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator mutation;
 for(uint32_t i=0;i<r.graph.elements.size();++i)for(const auto&a:r.graph.elements[i].attributes)if(a.type==15&&a.values.size()>=2&&a.values[0].reference!=0xfffffffe&&a.values[1].reference!=0xfffffffe){
  auto owner=r.elements.graphHandles[i];auto name=r.graph.strings[a.name];auto handle=r.attributes.find(owner,name);std::vector<uint32_t> old{a.values[0].reference,a.values[1].reference};
  uint32_t id=old[0]==0?1:0;auto target=r.elements.graphHandles[id];auto before=r.references.occurrences(target,handle);size_t oldCount=(old[0]==id)+(old[1]==id);std::vector<int> phases;
  auto identity=r.attributes.identity(r.graph,r.elements,handle);auto&ownerState=mutation.owners[owner];setElementNotifications(ownerState.state,true);ownerState.attributeFlags[identity]=initialAttributeFlags(a.type)|0x2100;mutation.cycle.phase=4;
  if(coordinatedReferenceRange(r,mutation,owner,name,0,{target,target},false,[](int,uint32_t,size_t,size_t){})||a.values[0].reference!=old[0]||!mutation.queues.marked.empty())throw std::runtime_error("constructor array restriction ignored");mutation.cycle.phase=2;
  auto callback=[&](int phase,uint32_t h,size_t start,size_t end){if(h!=handle||start!=0||end!=1)throw std::runtime_error("range callback metadata mismatch");phases.push_back(phase);
   if(phase==0&&(a.values[0].reference!=old[0]||a.values[1].reference!=old[1]||r.references.occurrences(target,handle)!=before))throw std::runtime_error("pre phase mismatch");
   if(phase==1&&(a.values[0].reference!=id||a.values[1].reference!=id||r.references.occurrences(target,handle)!=before-oldCount))throw std::runtime_error("range phase mismatch");
   if(phase==2&&r.references.occurrences(target,handle)!=before-oldCount+2)throw std::runtime_error("post phase mismatch");};
  if(!coordinatedReferenceRange(r,mutation,owner,name,0,{target,target},false,callback)||phases!=std::vector<int>{0,1,2})throw std::runtime_error("range phases failed");phases.clear();
  if(coordinatedReferenceRange(r,mutation,owner,name,0,{target,target},false,callback)||coordinatedReferenceRange(r,mutation,owner,name,a.values.size(),{target},false,callback)||coordinatedReferenceRange(r,mutation,owner,name,0,{},false,callback)||!phases.empty())throw std::runtime_error("invalid/unchanged range notified");
  mutation.cycle.phase=3;if(coordinatedReferenceRange(r,mutation,owner,name,0,{0xffffffff,0xffffffff},false,callback)||a.values[0].reference!=id||!phases.empty())throw std::runtime_error("rejected range changed graph");
  mutation.cycle.phase=2;setElementNotifications(ownerState.state,false);
  if(!coordinatedReferenceRange(r,mutation,owner,name,0,{0xffffffff,0xffffffff},false,callback)||!phases.empty()||r.references.occurrences(target,handle)!=before-oldCount)throw std::runtime_error("silent clear mismatch");
  setElementNotifications(ownerState.state,true);unsigned dynamicCalls=0;
  if(!coordinatedReferenceRange(r,mutation,owner,name,0,{target,target},false,[&](int phase,uint32_t,size_t,size_t){if(phase!=0)throw std::runtime_error("disabled later phase called");++dynamicCalls;ownerState.attributeFlags[identity]&=uint16_t(~0x100);})||dynamicCalls!=1||r.references.occurrences(target,handle)!=before-oldCount+2)throw std::runtime_error("dynamic notification predicate mismatch");
  std::cout<<"coordinated_range_phase_checks=3 rejection_checks=5 flag_gated_clear_checks=1 dynamic_flag_checks=1\n";return 0;
 }throw std::runtime_error("range fixture missing");
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
