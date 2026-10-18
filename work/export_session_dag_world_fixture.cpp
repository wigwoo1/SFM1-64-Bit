#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "live_session_scene_candidate.h"
#include "session_render_root_candidate.h"
#include "session_model_illumination_dag_candidate.h"
#include "dag_world_fixture.h"
#include <iostream>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 export required");if(argc!=3)throw std::runtime_error("session.dmx fixtures.bin required");
    auto source=loadGraph(argv[1]);const auto root=sessionRenderRootCandidate(source);RetainedShotTimelineCandidate timeline(std::move(source),root);
    std::ofstream out(argv[2],std::ios::binary);if(!out)throw std::runtime_error("fixture output");size_t snapshots=0,total=0;
    // Select samples from the actual indexed shot containing tick 50000.
    // Nearby timeline entries may intentionally have no renderable scene.
    const auto targetShot=timeline.selectShot(50000).clip;std::vector<int32_t>available;
    for(int32_t tick=40000;tick<=65000;tick+=100){const auto classified=timeline.classifyFrame(tick);if(classified.state==FilmTimelineStateCandidate::SceneClip&&classified.shot&&classified.shot->clip==targetShot)available.push_back(tick);}
    if(available.size()<6)throw std::runtime_error("insufficient indexed shot sample coverage");
    std::set<int32_t>ticks{50000};for(size_t i=0;i<5;++i)ticks.insert(available[i*(available.size()-1)/4]);
    for(auto tick:available){if(ticks.size()==6)break;ticks.insert(tick);}
    if(ticks.size()!=6)throw std::runtime_error("unexpected sample selection");
    for(int32_t tick:ticks){
        const auto step=evaluateLiveSessionSceneCandidate(timeline,tick,1280,720);const auto&g=timeline.graph();const auto binding=readFilmSceneRenderBindingCandidate(g,step.scene.shot);GraphTransformHierarchy hierarchy(g,binding.scene);
        std::set<uint32_t>scope;std::vector<uint32_t>scopePending{binding.scene};while(!scopePending.empty()){const auto id=scopePending.back();scopePending.pop_back();if(!scope.insert(id).second)continue;for(auto child:hierarchyRefs(g,g.elements.at(id),"children",15))scopePending.push_back(child);}
        for(const auto&model:step.scene.models){
            const auto selected=sessionModelIlluminationDagCandidate(g,model.id,[&](uint32_t id){if(g.strings.at(g.elements.at(id).type)!="DmeDag")throw std::runtime_error("illumination DAG ancestry unverified");return true;});
            if(!selected.dag)continue;
            std::set<uint32_t>closure;std::vector<uint32_t>pending{*selected.dag};
            while(!pending.empty()){const auto id=pending.back();pending.pop_back();if(!closure.insert(id).second)continue;
                if(closure.size()>64)throw std::runtime_error("fixture closure limit");
                if(!scope.count(id))throw std::runtime_error("external DAG parent scope unverified");
                const auto&type=g.strings.at(g.elements.at(id).type);if(type!="DmeDag"&&type!="DmeGameModel")throw std::runtime_error("ancestor DAG class unverified: "+type);
                auto ordinary=hierarchy.ordinaryParents.find(id);if(ordinary!=hierarchy.ordinaryParents.end())pending.push_back(ordinary->second);
                const auto over=hierarchySingleReference(g,g.elements.at(id),"overrideParent");if(over!=UINT32_MAX)pending.push_back(over);
            }
            std::map<uint32_t,uint32_t>mapping;for(auto id:closure)mapping.emplace(id,uint32_t(mapping.size()));
            auto remap=[&](uint32_t id){return id==UINT32_MAX?id:mapping.at(id);};std::vector<DagWorldFixtureNode>nodes(closure.size());
            for(auto id:closure){auto&n=nodes[mapping.at(id)];n.flags=0;const auto&e=g.elements.at(id);auto parent=hierarchy.ordinaryParents.find(id);n.parent=parent==hierarchy.ordinaryParents.end()?UINT32_MAX:remap(parent->second);n.overrideParent=remap(hierarchySingleReference(g,e,"overrideParent"));
                auto flag=[&](const char*name,uint32_t present,uint32_t truth){const auto*a=namedAttribute(g,e,name);if(!a)return;if(a->type!=4||a->values.size()!=1||a->values[0].wire.size()!=1)throw std::runtime_error("fixture boolean schema");n.flags|=present;if(a->values[0].wire[0])n.flags|=truth;};
                flag("disableOverride",0,1);flag("overridePos",2,4);flag("overrideRot",8,16);
                const auto transform=hierarchySingleReference(g,e,"transform");if(transform!=UINT32_MAX){n.flags|=32;const auto&t=g.elements.at(transform);if(g.strings.at(t.type)!="DmeTransform")throw std::runtime_error("fixture transform type");
                    auto copy=[&](const char*name,uint8_t type,void*dest,size_t bytes){const auto*a=namedAttribute(g,t,name);if(!a||a->type!=type||a->values.size()!=1||a->values[0].wire.size()!=bytes)throw std::runtime_error("fixture numeric schema");std::memcpy(dest,a->values[0].wire.data(),bytes);};copy("position",10,n.position.data(),12);copy("orientation",13,n.orientation.data(),16);
                    if(namedAttribute(g,t,"scale")){copy("scale",3,&n.scale,4);n.flags|=64;}
                }
            }
            // Verify normalization retained the existing evaluator's matrices and
            // selected parent IDs before using it as original-code input.
            const auto normalized=dagWorldFixtureGraph(nodes);GraphTransformHierarchy normalizedHierarchy(normalized);
            for(auto id:closure){const auto actual=hierarchy.world(id),normalizedMatrix=normalizedHierarchy.world(mapping.at(id));if(std::memcmp(actual.data(),normalizedMatrix.data(),48))throw std::runtime_error("fixture normalization changed world matrix");if(remap(hierarchy.parent(id,true))!=normalizedHierarchy.parent(mapping.at(id),true)||remap(hierarchy.parent(id,false))!=normalizedHierarchy.parent(mapping.at(id),false))throw std::runtime_error("fixture normalization changed parents");}
            const uint32_t count=uint32_t(nodes.size());out.write(reinterpret_cast<const char*>(&count),4);out.write(reinterpret_cast<const char*>(nodes.data()),std::streamsize(nodes.size()*sizeof(nodes[0])));++snapshots;total+=nodes.size();
            std::cout<<"tick="<<tick<<" shot="<<step.scene.shot<<" model="<<model.id<<" selected_dag="<<*selected.dag<<" normalized_index="<<mapping.at(*selected.dag)<<" nodes="<<count<<" source_ids=";for(auto id:closure)std::cout<<id<<',';std::cout<<'\n';
        }
    }
    out.flush();if(!out||!snapshots)throw std::runtime_error("empty/failed fixture output");std::cout<<"PASS snapshots="<<snapshots<<" nodes="<<total<<" native_live_inputs=UNVERIFIED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
