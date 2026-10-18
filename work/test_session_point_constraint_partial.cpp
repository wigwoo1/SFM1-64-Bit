#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_point_constraint_runtime.h"
int main(int argc,char**argv){try{
    if(argc!=3)return 2;auto g=loadGraph(argv[1]);unsigned checks=0,changed=0;
    for(uint32_t id=0;id<g.elements.size();++id){
        if(g.strings[g.elements[id].type]!="DmeRigPointConstraintOperator")continue;
        const auto targetIds=hierarchyRefs(g,g.elements[id],"targets",15);
        const auto slaveId=hierarchyRefs(g,g.elements[id],"slave",1).at(0);
        const auto dag=hierarchyRefs(g,g.elements[slaveId],"target",1).at(0);
        const auto transform=hierarchyRefs(g,g.elements[dag],"transform",1).at(0);
        unsigned live=0;for(auto target:targetIds)if(!hierarchyRefs(g,g.elements[target],"target",1).empty())++live;
        if(!live)throw std::runtime_error("no live partial target");
        for(float influence:{0.0f,.25f,.75f}){
            for(auto target:targetIds){auto* weight=runtimeAttribute(g,target,"targetWeight");setFloat(*weight,0,influence/float(live));}
            GraphTransformHierarchy h(g);auto average=graphPointConstraintAverage(g,h,id,{0,0,0});
            if(!(average.influence>=0 && average.influence<1))throw std::runtime_error("partial influence not reached");
            const auto* sp=namedAttribute(g,g.elements[slaveId],"position");const auto* sq=namedAttribute(g,g.elements[slaveId],"orientation");
            std::array<float,3> p{};std::array<float,4> q{};
            std::memcpy(p.data(),sp->values[0].wire.data(),12);std::memcpy(q.data(),sq->values[0].wire.data(),16);
            const auto pp=h.parent(dag,true),rp=h.parent(dag,false);
            const auto pm=h.world(pp),rm=h.world(rp);
            auto slaveWorld=convertRecoveredLocalToWorld(recoveredQuaternionTransformMatrix(q,p),pm,rm,pp==rp);
            std::array<float,3> destination{};
            for(unsigned c=0;c<3;++c)destination[c]=average.position[c]*average.influence+slaveWorld[c][3]*(1.0f-average.influence);
            auto expected=h.world(dag);for(unsigned c=0;c<3;++c)expected[c][3]=destination[c];
            expected=convertRecoveredWorldToLocal(expected,pm,recoveredTransformScaleCandidate(pm),rm,recoveredTransformScaleCandidate(rm),pp==rp);
            const auto originalOrientation=runtimeAttribute(g,transform,"orientation")->values[0].wire;
            SessionAttributeChanges journal;
            if(!executeSessionPointConstraint(g,h,id,{0,0,0},&journal))throw std::runtime_error("partial point skipped");
            if(journal.attributes.size()>1)throw std::runtime_error("extra partial notifications");
            changed+=unsigned(journal.attributes.size());
            const auto* actual=runtimeAttribute(g,transform,"position");
            for(unsigned c=0;c<3;++c)if(std::memcmp(actual->values[0].wire.data()+c*4,&expected[c][3],4))throw std::runtime_error("partial local position mismatch");
            if(runtimeAttribute(g,transform,"orientation")->values[0].wire!=originalOrientation)throw std::runtime_error("partial orientation changed");
            ++checks;
        }
    }
    saveGraph(g,argv[2]);auto r=loadGraph(argv[2]);GraphTransformHierarchy rh(r);unsigned replays=0;
    for(uint32_t id=0;id<r.elements.size();++id)if(r.strings[r.elements[id].type]=="DmeRigPointConstraintOperator"){
        if(!executeSessionPointConstraint(r,rh,id,{0,0,0}))throw std::runtime_error("reloaded partial skipped");++replays;
    }
    std::cout<<"partial_point_checks="<<checks<<" changed_attributes="<<changed<<" reload_partial_executions="<<replays<<std::endl;
    return checks==120&&replays==40?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
