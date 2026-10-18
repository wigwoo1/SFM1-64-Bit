#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_runtime.h"
int main(int argc,char**argv){try{if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));
 for(uint32_t i=0;i<r.graph.elements.size();++i)for(const auto&a:r.graph.elements[i].attributes)if(a.type==15&&a.values.size()>=2&&a.values[0].reference!=0xfffffffe&&a.values[1].reference!=0xfffffffe){
  auto owner=r.elements.graphHandles[i];auto name=r.graph.strings[a.name];auto handle=r.attributes.find(owner,name);std::vector<uint32_t> old{a.values[0].reference,a.values[1].reference};
  uint32_t id=old[0]==0?1:0;auto target=r.elements.graphHandles[id];auto before=r.references.occurrences(target,handle);size_t oldCount=(old[0]==id)+(old[1]==id);std::vector<int> phases;
  auto callback=[&](int phase,uint32_t h,size_t start,size_t end){if(h!=handle||start!=0||end!=1)throw std::runtime_error("range callback metadata mismatch");phases.push_back(phase);
   if(phase==0&&(a.values[0].reference!=old[0]||a.values[1].reference!=old[1]||r.references.occurrences(target,handle)!=before))throw std::runtime_error("pre phase mismatch");
   if(phase==1&&(a.values[0].reference!=id||a.values[1].reference!=id||r.references.occurrences(target,handle)!=before-oldCount))throw std::runtime_error("range phase mismatch");
   if(phase==2&&r.references.occurrences(target,handle)!=before-oldCount+2)throw std::runtime_error("post phase mismatch");};
  if(!r.setReferenceRange(owner,name,0,{target,target},true,callback)||phases!=std::vector<int>{0,1,2})throw std::runtime_error("range phases failed");phases.clear();
  if(r.setReferenceRange(owner,name,0,{target,target},true,callback)||r.setReferenceRange(owner,name,a.values.size(),{target},true,callback)||r.setReferenceRange(owner,name,0,{},true,callback)||!phases.empty())throw std::runtime_error("invalid/unchanged range notified");
  if(!r.setReferenceRange(owner,name,0,{0xffffffff,0xffffffff},false,callback)||!phases.empty()||r.references.occurrences(target,handle)!=before-oldCount)throw std::runtime_error("silent clear mismatch");
  std::cout<<"range_phase_checks=3 rejection_checks=3 silent_clear_checks=1\n";return 0;
 }throw std::runtime_error("range fixture missing");
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
