#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_orientation_constraint_runtime.h"
int main(int argc,char**argv){try{
    if(argc!=3)return 2;
    auto g=loadGraph(argv[1]);GraphTransformHierarchy h(g);
    unsigned executed=0,changed=0;
    for(uint32_t id=0;id<g.elements.size();++id)
        if(g.strings[g.elements[id].type]=="DmeRigOrientConstraintOperator"){
            auto slaves=hierarchyRefs(g,g.elements[id],"slave",1);
            auto dags=hierarchyRefs(g,g.elements.at(slaves.at(0)),"target",1);
            auto ts=hierarchyRefs(g,g.elements.at(dags.at(0)),"transform",1);
            const auto before=namedAttribute(g,g.elements.at(ts.at(0)),"position")->values[0].wire;
            SessionAttributeChanges journal;
            if(!executeSessionOrientationConstraint(g,h,id,{0,0,0,1},&journal))throw std::runtime_error("orientation skipped");
            if(journal.attributes.size()>1)throw std::runtime_error("duplicate journal entry");
            for(const auto& entry:journal.attributes)
                if(entry.first!=ts[0]||entry.second!="orientation")throw std::runtime_error("wrong journal destination");
            if(before!=namedAttribute(g,g.elements.at(ts[0]),"position")->values[0].wire)
                throw std::runtime_error("position changed");
            changed+=unsigned(journal.attributes.size());++executed;
        }
    if(!executed)throw std::runtime_error("no orientations");
    saveGraph(g,argv[2]);auto r=loadGraph(argv[2]);GraphTransformHierarchy rh(r);
    unsigned replayed=0;
    for(uint32_t id=0;id<r.elements.size();++id)
        if(r.strings[r.elements[id].type]=="DmeRigOrientConstraintOperator"){
            if(!executeSessionOrientationConstraint(r,rh,id,{0,0,0,1}))throw std::runtime_error("reload skipped");
            ++replayed;
        }
    std::cout<<"orientation_executions="<<executed<<" changed_attributes="<<changed
        <<" reload_executions="<<replayed<<std::endl;
    return executed==replayed?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
