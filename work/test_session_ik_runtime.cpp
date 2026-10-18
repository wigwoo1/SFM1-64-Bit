#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_ik_runtime.h"
int main(int argc,char**argv){try{if(argc!=3)return 2;auto g=loadGraph(argv[1]);GraphTransformHierarchy h(g);
 unsigned executed=0,changed=0;
 for(uint32_t id=0;id<g.elements.size();++id)if(g.strings[g.elements[id].type]=="DmeRigIKConstraintOperator"){
  const auto inputs=readRecoveredGraphIkInputs(g,id);std::array<std::vector<char>,3>before{};
  for(unsigned i=0;i<3;++i){auto t=hierarchyRefs(g,g.elements[inputs.dags[i]],"transform",1).at(0);before[i]=namedAttribute(g,g.elements[t],"position")->values[0].wire;}
  SessionAttributeChanges journal;if(!executeSessionIkCandidate(g,h,id,{0,0,0},{nullptr,nullptr,nullptr},&journal))throw std::runtime_error("IK skipped");
  if(journal.attributes.size()>2)throw std::runtime_error("extra IK journal entries");
  for(const auto&entry:journal.attributes)if(entry.second!="orientation")throw std::runtime_error("unexpected IK mutation");
  for(unsigned i=0;i<3;++i){auto t=hierarchyRefs(g,g.elements[inputs.dags[i]],"transform",1).at(0);if(before[i]!=namedAttribute(g,g.elements[t],"position")->values[0].wire)throw std::runtime_error("IK position changed without defaults");}
  changed+=unsigned(journal.attributes.size());++executed;
 }
 saveGraph(g,argv[2]);auto r=loadGraph(argv[2]);GraphTransformHierarchy rh(r);unsigned replays=0;
 for(uint32_t id=0;id<r.elements.size();++id)if(r.strings[r.elements[id].type]=="DmeRigIKConstraintOperator"){
  if(!executeSessionIkCandidate(r,rh,id,{0,0,0},{nullptr,nullptr,nullptr}))throw std::runtime_error("reload skipped");++replays;}
 std::cout<<"ik_executions="<<executed<<" changed_attributes="<<changed<<" reload_executions="<<replays<<std::endl;
 return executed==16&&replays==16?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
