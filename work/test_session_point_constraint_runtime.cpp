#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_point_constraint_runtime.h"
int main(int argc,char**argv){try{if(argc!=3)return 2;auto g=loadGraph(argv[1]);GraphTransformHierarchy h(g);unsigned executed=0,changes=0;for(uint32_t id=0;id<g.elements.size();++id)if(g.strings[g.elements[id].type]=="DmeRigPointConstraintOperator"){SessionAttributeChanges journal;if(!executeSessionPointConstraint(g,h,id,{0,0,0},&journal))throw std::runtime_error("point skipped");changes+=unsigned(journal.attributes.size());++executed;}if(!executed)throw std::runtime_error("no points");saveGraph(g,argv[2]);auto r=loadGraph(argv[2]);GraphTransformHierarchy rh(r);unsigned replays=0;for(uint32_t id=0;id<r.elements.size();++id)if(r.strings[r.elements[id].type]=="DmeRigPointConstraintOperator"){if(!executeSessionPointConstraint(r,rh,id,{0,0,0}))throw std::runtime_error("reload point skipped");++replays;}std::cout<<"point_executions="<<executed<<" changed_attributes="<<changes<<" reload_executions="<<replays<<std::endl;return replays==executed?0:1;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
