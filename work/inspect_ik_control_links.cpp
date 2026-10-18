#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "graph_ik_inputs.h"
#include "graph_ik_parent_frame.h"
#include "graph_ik_solution.h"
int main(int argc,char**argv){try{
 if(argc!=2)return 2;const auto g=loadGraph(argv[1]);
 std::map<uint32_t,std::vector<uint32_t>> owners;
 for(uint32_t id=0;id<g.elements.size();++id)for(const auto&a:g.elements[id].attributes)
  if(a.type==1||a.type==15)for(const auto&v:a.values)if(v.reference!=0xffffffffu)owners[v.reference].push_back(id);
 unsigned ik=0,slaves=0,missing=0,ambiguous=0;GraphTransformHierarchy hierarchy(g);
 for(uint32_t id=0;id<g.elements.size();++id){if(g.strings[g.elements[id].type]!="DmeRigIKConstraintOperator")continue;
  const auto inputs=readRecoveredGraphIkInputs(g,id);
  for(auto dag:inputs.dags)if(dag==0xffffffffu)throw std::runtime_error("missing IK joint DAG");
  const auto frame=graphIkParentFrameCandidate(g,hierarchy,inputs.dags[0]);
  for(const auto&row:frame)for(float value:row)if(!std::isfinite(value))throw std::runtime_error("nonfinite IK parent frame");
  const auto solution=graphIkSolutionCandidate(g,hierarchy,id,{0,0,0},{nullptr,nullptr,nullptr});
  for(const auto&q:{solution.rotations.start,solution.rotations.mid})for(float value:q)
   if(!std::isfinite(value))throw std::runtime_error("nonfinite graph IK solution");
  ++ik;std::cout<<"ik "<<id<<" pvTarget="<<inputs.poleVectorTarget<<std::endl;
  for(const auto&a:g.elements[id].attributes)if(a.type==1){for(const auto&v:a.values){
   if(v.reference==0xffffffffu||g.strings[g.elements.at(v.reference).type]!="DmeConstraintSlave")continue;
   ++slaves;const auto dags=hierarchyRefs(g,g.elements[v.reference],"target",1);
   if(dags.empty()){std::cout<<" slave_field="<<g.strings[a.name]<<" missing_dag"<<std::endl;++missing;continue;}
   const auto dag=dags[0];
   std::vector<uint32_t> candidates;
   const auto transforms=hierarchyRefs(g,g.elements[dag],"transform",1);
   const auto append=[&](uint32_t target){for(auto owner:owners[target])if(g.strings[g.elements[owner].type]=="DmeChannel"){
    const auto refs=hierarchyRefs(g,g.elements[owner],"transform",1);
    for(auto ref:refs)if(g.strings[g.elements[ref].type]=="DmeTransformControl")candidates.push_back(ref);
   }};
   for(auto t:transforms)append(t);
   for(auto owner:owners[dag])if(g.strings[g.elements[owner].type]=="DmeConstraintSlave")append(owner);
   std::set<uint32_t> unique(candidates.begin(),candidates.end());
   if(unique.empty())++missing;if(unique.size()>1)++ambiguous;
   std::cout<<" slave_field="<<g.strings[a.name]<<" dag="<<dag<<" controls=";
   for(auto c:unique)std::cout<<c<<",";std::cout<<std::endl;
  }}
 }
 std::cout<<"ik="<<ik<<" slaves="<<slaves<<" missing_controls="<<missing<<" ambiguous_controls="<<ambiguous<<std::endl;
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
