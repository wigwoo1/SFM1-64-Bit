#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "graph_point_constraint.h"
int main(int argc,char**argv){try{
    if(argc!=2)return 2;auto g=loadGraph(argv[1]);
    std::map<uint32_t,std::set<uint32_t>> parents;
    std::map<std::string,unsigned> dagTypes;
    unsigned transforms=0,dagNodes=0,overrideNodes=0,links=0,multiple=0;
    for(uint32_t i=0;i<g.elements.size();++i){const auto&e=g.elements[i];
        if(g.strings[e.type]=="DmeTransform"){
            const auto m=graphLocalTransformMatrix(g,i);++transforms;
            const auto* p=namedAttribute(g,e,"position");
            for(unsigned c=0;c<3;++c)if(std::memcmp(&m[c][3],p->values[0].wire.data()+c*4,4))throw std::runtime_error("translation mismatch");
        }
        if(!namedAttribute(g,e,"transform"))continue;
        ++dagNodes;++dagTypes[g.strings[e.type]];
        for(auto ref:hierarchyRefs(g,e,"children",15)){parents[ref].insert(i);++links;}
        if(namedAttribute(g,e,"overrideParent") || namedAttribute(g,e,"overridePos") || namedAttribute(g,e,"overrideRot")){++overrideNodes;std::cout<<"override_node "<<i<<" type="<<g.strings[e.type];for(const auto&a:e.attributes)if(g.strings[a.name].find("override")==0){std::cout<<" "<<g.strings[a.name]<<" type="<<unsigned(a.type);for(const auto&v:a.values){std::cout<<" ref="<<v.reference<<" wire=";for(unsigned char b:v.wire)std::cout<<unsigned(b)<<",";}}std::cout<<std::endl;}
    }
    for(const auto& p:parents)if(p.second.size()>1)++multiple;
    GraphTransformHierarchy hierarchy(g);unsigned worldChecks=0;for(uint32_t i=0;i<g.elements.size();++i)if(namedAttribute(g,g.elements[i],"transform")){hierarchy.world(i);++worldChecks;}std::cout<<"world_matrices="<<worldChecks<<std::endl;
    unsigned pointChecks=0;for(uint32_t i=0;i<g.elements.size();++i)if(g.strings[g.elements[i].type]=="DmeRigPointConstraintOperator"){auto result=graphPointConstraintAverage(g,hierarchy,i,{0,0,0});std::cout<<"point_average "<<i<<" influence="<<result.influence<<std::endl;++pointChecks;}std::cout<<"point_constraints="<<pointChecks<<std::endl;
    for(const auto&t:dagTypes)std::cout<<"dag_type "<<t.first<<"="<<t.second<<std::endl;
    std::cout<<"local_transforms="<<transforms<<" dag_nodes="<<dagNodes<<" child_links="<<links<<" multiple_parents="<<multiple<<" override_nodes="<<overrideNodes<<std::endl;
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
