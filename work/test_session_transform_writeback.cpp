#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_transform_runtime.h"
int main(int argc,char**argv){try{
    if(argc!=3)return 2;auto g=loadGraph(argv[1]);GraphTransformHierarchy hierarchy(g);
    unsigned writes=0,roots=0,attempts=0;
    for(uint32_t id=0;id<g.elements.size();++id){
        if(!namedAttribute(g,g.elements[id],"transform"))continue;
        auto refs=hierarchyRefs(g,g.elements[id],"transform",1);if(refs.empty())continue;
        auto* orientation=runtimeAttribute(g,refs[0],"orientation");auto saved=orientation->values[0].wire;
        std::array<float,3> requested{123.25f+float(id%13),-47.5f,8.125f};
        SessionAttributeChanges changes;
        ++attempts;if(!writeSessionWorldPosition(g,hierarchy,id,requested,&changes)){if(!changes.attributes.empty())throw std::runtime_error("unchanged position notified");continue;}
        if(changes.attributes.size()!=1 || changes.attributes[0].first!=refs[0] || changes.attributes[0].second!="position")throw std::runtime_error("incorrect write journal");
        if(orientation->values[0].wire!=saved)throw std::runtime_error("orientation changed");
        if(hierarchy.parent(id,true)==0xffffffff && hierarchy.parent(id,false)==0xffffffff){
            auto* p=runtimeAttribute(g,refs[0],"position");if(std::memcmp(p->values[0].wire.data(),requested.data(),12))throw std::runtime_error("root write mismatch");++roots;
        }
        ++writes;
    }
    saveGraph(g,argv[2]);auto reloaded=loadGraph(argv[2]);GraphTransformHierarchy reloadHierarchy(reloaded);unsigned evaluated=0;
    for(uint32_t i=0;i<reloaded.elements.size();++i)if(namedAttribute(reloaded,reloaded.elements[i],"transform")){reloadHierarchy.world(i);++evaluated;}
    if(!writes||evaluated!=attempts)throw std::runtime_error("reload evaluation mismatch");
    std::cout<<"world_position_writes="<<writes<<" independent_root_checks="<<roots<<" reload_world_matrices="<<evaluated<<std::endl;return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
