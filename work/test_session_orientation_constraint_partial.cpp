#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_orientation_constraint_runtime.h"
int main(int argc,char**argv){try{
    if(argc!=3)return 2;auto g=loadGraph(argv[1]);unsigned checks=0,changed=0;
    for(uint32_t id=0;id<g.elements.size();++id){
        if(g.strings[g.elements[id].type]!="DmeRigOrientConstraintOperator")continue;
        const auto targetIds=hierarchyRefs(g,g.elements[id],"targets",15);
        const auto slaveId=hierarchyRefs(g,g.elements[id],"slave",1).at(0);
        const auto dag=hierarchyRefs(g,g.elements[slaveId],"target",1).at(0);
        const auto transform=hierarchyRefs(g,g.elements[dag],"transform",1).at(0);
        unsigned live=0;for(auto target:targetIds)if(!hierarchyRefs(g,g.elements[target],"target",1).empty())++live;
        if(!live)throw std::runtime_error("no live orientation target");
        for(float influence:{0.0f,.25f,.75f}){
            for(auto target:targetIds)setFloat(*runtimeAttribute(g,target,"targetWeight"),0,influence/float(live));
            GraphTransformHierarchy h(g);
            const auto average=graphOrientationConstraintAverage(g,h,id,{0,0,0,1});
            if(!(average.influence>=0 && average.influence<1))throw std::runtime_error("partial influence not reached");
            const auto* sp=namedAttribute(g,g.elements[slaveId],"position");
            const auto* sq=namedAttribute(g,g.elements[slaveId],"orientation");
            std::array<float,3> p{};std::array<float,4> q{};
            std::memcpy(p.data(),sp->values[0].wire.data(),12);std::memcpy(q.data(),sq->values[0].wire.data(),16);
            const auto pp=h.parent(dag,true),rp=h.parent(dag,false);
            const auto world=convertRecoveredLocalToWorld(recoveredQuaternionTransformMatrix(q,p),h.world(pp),h.world(rp),pp==rp);
            const auto blended=interpolateRecoveredQuaternion(extractRecoveredTransformOrientationCandidate(world),average.orientation,average.influence,false);
            const auto expected=multiplyRecoveredAlignedQuaternions(invertRecoveredQuaternion(h.worldOrientation(rp)),blended);
            const auto originalPosition=runtimeAttribute(g,transform,"position")->values[0].wire;
            SessionAttributeChanges journal;
            if(!executeSessionOrientationConstraint(g,h,id,{0,0,0,1},&journal))throw std::runtime_error("partial orientation skipped");
            if(journal.attributes.size()>1)throw std::runtime_error("extra notifications");
            for(const auto& entry:journal.attributes)
                if(entry.first!=transform||entry.second!="orientation")throw std::runtime_error("wrong notification");
            changed+=unsigned(journal.attributes.size());
            if(std::memcmp(runtimeAttribute(g,transform,"orientation")->values[0].wire.data(),expected.data(),16))
                throw std::runtime_error("partial orientation mismatch");
            if(runtimeAttribute(g,transform,"position")->values[0].wire!=originalPosition)throw std::runtime_error("position changed");
            if(h.cache.size())throw std::runtime_error("stale hierarchy cache");
            ++checks;
        }
    }
    saveGraph(g,argv[2]);auto r=loadGraph(argv[2]);GraphTransformHierarchy rh(r);unsigned replays=0;
    for(uint32_t id=0;id<r.elements.size();++id)if(r.strings[r.elements[id].type]=="DmeRigOrientConstraintOperator"){
        if(!executeSessionOrientationConstraint(r,rh,id,{0,0,0,1}))throw std::runtime_error("reload skipped");++replays;
    }
    std::cout<<"partial_orientation_checks="<<checks<<" changed_attributes="<<changed<<" reload_partial_executions="<<replays<<std::endl;
    return checks==168&&replays==56?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
