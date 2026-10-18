#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_ik_runtime.h"
int main(int argc,char**argv){try{if(argc!=3)return 2;auto base=loadGraph(argv[1]);unsigned checks=0;std::vector<uint32_t>operators;
for(uint32_t id=0;id<base.elements.size();++id)if(base.strings[base.elements[id].type]=="DmeRigIKConstraintOperator")operators.push_back(id);
std::array<uint32_t,3>controls{};const std::array<std::array<float,3>,3>values{{{.25f,-.5f,.125f},{1.25f,0,0},{.875f,0,0}}};
for(unsigned i=0;i<3;++i){Element e{};e.type=internString(base,"DmeTransformControl");e.name=internString(base,"selected_control_test_"+std::to_string(i));e.id.fill(char(-89));std::memcpy(e.id.data(),&i,4);Value v;v.wire.resize(12);std::memcpy(v.wire.data(),values[i].data(),12);e.attributes.push_back({internString(base,"defaultPosition"),10,{v}});controls[i]=uint32_t(base.elements.size());base.elements.push_back(e);}
for(auto id:operators)for(unsigned mask=0;mask<8;++mask)for(float influence:{0.f,.25f,.75f}){
auto actual=base;auto expected=base;auto selected=controls;std::array<const std::array<float,3>*,3>defaults{};
for(unsigned i=0;i<3;++i){if(mask&(1u<<i))defaults[i]=&values[i];else selected[i]=0xffffffffu;}
const auto targets=hierarchyRefs(actual,actual.elements[id],"targets",15);unsigned live=0;for(auto t:targets)if(!hierarchyRefs(actual,actual.elements[t],"target",1).empty())++live;if(!live)throw std::runtime_error("missing targets");
for(auto t:targets){setFloat(*runtimeAttribute(actual,t,"targetWeight"),0,influence/float(live));setFloat(*runtimeAttribute(expected,t,"targetWeight"),0,influence/float(live));}
GraphTransformHierarchy ah(actual),eh(expected);SessionAttributeChanges aj,ej;
if(!executeSessionIkWithSelectedControlsCandidate(actual,ah,id,{0,0,0},selected,&aj)||!executeSessionIkCandidate(expected,eh,id,{0,0,0},defaults,&ej))throw std::runtime_error("IK skipped");
if(aj.attributes!=ej.attributes)throw std::runtime_error("journal mismatch");if(!ah.cache.empty())throw std::runtime_error("stale cache");
const auto inputs=readRecoveredGraphIkInputs(actual,id);for(unsigned i=0;i<3;++i){const auto t=hierarchyRefs(actual,actual.elements[inputs.dags[i]],"transform",1).at(0);for(const char*name:{"position","orientation"})if(namedAttribute(actual,actual.elements[t],name)->values[0].wire!=namedAttribute(expected,expected.elements[t],name)->values[0].wire)throw std::runtime_error("selected control IK mismatch");}
++checks;}
// Selected controls with absent default attributes must act like shared zero.
auto missing=base;for(auto c:controls)missing.elements[c].attributes.clear();for(auto id:operators){GraphTransformHierarchy h(missing);executeSessionIkWithSelectedControlsCandidate(missing,h,id,{0,0,0},controls);}
saveGraph(missing,argv[2]);const auto reload=loadGraph(argv[2]);for(auto c:controls)if(graphSelectedControlDefaultPositionCandidate(reload,c,{0,0,0})!=std::array<float,3>{0,0,0})throw std::runtime_error("missing default reload mismatch");
std::cout<<"selected_control_ik_comparisons="<<checks<<" missing_default_executions="<<operators.size()<<" reloaded_controls=3"<<std::endl;return checks==384?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
