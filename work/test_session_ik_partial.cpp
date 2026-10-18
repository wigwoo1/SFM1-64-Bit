#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_ik_runtime.h"
int main(int argc,char**argv){try{if(argc!=3)return 2;auto g=loadGraph(argv[1]);unsigned checks=0,changed=0;
 for(uint32_t id=0;id<g.elements.size();++id)if(g.strings[g.elements[id].type]=="DmeRigIKConstraintOperator"){
  const auto inputs=readRecoveredGraphIkInputs(g,id);const auto targets=hierarchyRefs(g,g.elements[id],"targets",15);
  unsigned live=0;for(auto t:targets)if(!hierarchyRefs(g,g.elements[t],"target",1).empty())++live;
  if(!live)throw std::runtime_error("no live IK target");
  for(float influence:{0.0f,.25f,.75f}){
   for(auto t:targets)setFloat(*runtimeAttribute(g,t,"targetWeight"),0,influence/float(live));
   const std::array<std::array<float,3>,3>defaults{{{.25f,-.5f,.125f},{1.25f,0,0},{.875f,0,0}}};
   const std::array<const std::array<float,3>*,3>selected{&defaults[0],&defaults[1],&defaults[2]};
   GraphTransformHierarchy h(g);SessionAttributeChanges journal;
   if(!executeSessionIkCandidate(g,h,id,{0,0,0},selected,&journal))throw std::runtime_error("partial IK skipped");
   if(journal.attributes.size()>5)throw std::runtime_error("extra IK notifications");
   for(unsigned i=0;i<3;++i){auto transform=hierarchyRefs(g,g.elements[inputs.dags[i]],"transform",1).at(0);
    if(std::memcmp(runtimeAttribute(g,transform,"position")->values[0].wire.data(),defaults[i].data(),12))throw std::runtime_error("IK default position mismatch");}
   for(const auto&e:journal.attributes)if(e.second!="position"&&e.second!="orientation")throw std::runtime_error("unexpected IK notification");
   if(!h.cache.empty())throw std::runtime_error("stale IK cache");changed+=unsigned(journal.attributes.size());++checks;
  }
 }
 saveGraph(g,argv[2]);const auto r=loadGraph(argv[2]);unsigned persisted=0;
 for(uint32_t id=0;id<g.elements.size();++id)if(g.strings[g.elements[id].type]=="DmeRigIKConstraintOperator"){
  const auto inputs=readRecoveredGraphIkInputs(g,id);
  for(auto dag:inputs.dags){auto t=hierarchyRefs(g,g.elements[dag],"transform",1).at(0);
   for(const char*name:{"position","orientation"})if(namedAttribute(g,g.elements[t],name)->values[0].wire!=namedAttribute(r,r.elements[t],name)->values[0].wire)throw std::runtime_error("IK persistence mismatch");}
  ++persisted;
 }
 std::cout<<"partial_ik_checks="<<checks<<" changed_attributes="<<changed<<" persisted_operators="<<persisted<<std::endl;
 return checks==48&&persisted==16?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
