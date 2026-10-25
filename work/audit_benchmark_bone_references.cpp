#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "live_session_scene_candidate.h"
#include "session_render_root_candidate.h"
#include "session_ifm_bone_references_candidate.h"
#include <iostream>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 audit");if(argc!=2&&argc!=4)throw std::runtime_error("session [normal-palette-queries fresh-output] required");
    std::ifstream normal;std::ofstream output;if(argc==4){if(std::filesystem::exists(argv[3]))throw std::runtime_error("fresh precompute export required");normal.open(argv[2],std::ios::binary);output.open(argv[3],std::ios::binary);if(!normal||!output)throw std::runtime_error("precompute export IO");}
    auto g=loadGraph(argv[1]);const auto root=sessionRenderRootCandidate(g);RetainedShotTimelineCandidate timeline(std::move(g),root);
    for(int32_t tick:{50000,50417}){
        const auto scene=evaluateLiveSessionSceneCandidate(timeline,tick,1280,720);const auto&graph=timeline.graph();
        const auto binding=readFilmSceneRenderBindingCandidate(graph,scene.scene.shot);GraphTransformHierarchy hierarchy(graph,binding.scene);
        size_t bones=0,references=0,overrides=0,ambiguous=0;
        for(const auto&instance:scene.scene.models){if(instance.placement.asset[0]=='*')continue;const auto*array=namedAttribute(graph,graph.elements.at(instance.id),"bones");if(!array||array->type!=15)throw std::runtime_error("bone schema");
            auto isDag=[&](uint32_t id){const auto&t=graph.strings.at(graph.elements.at(id).type);return t=="DmeDag"||t=="DmeGameModel";};
            const auto snapshot=sessionIfmBoneReferencesCandidate(graph,instance.id,uint32_t(array->values.size()),isDag,[&](uint32_t id){return graph.strings.at(graph.elements.at(id).type)=="DmeTransform";},[&](uint32_t id){return hierarchy.world(id);});
            const auto pre=ifmBonePrecomputeCandidate(true,true,snapshot);size_t writes=0;for(const auto&p:pre.palette)writes+=bool(p);
            std::cout<<"selection tick="<<tick<<" model="<<instance.id<<" count="<<snapshot.size()<<" precomputed="<<writes<<'\n';
            if(argc==4){std::array<uint32_t,2>h{};normal.read(reinterpret_cast<char*>(h.data()),8);if(!normal||h[0]!=snapshot.size()||h[1]!=0x7ff00)throw std::runtime_error("benchmark normal query binding");std::vector<char>pose(48+h[0]*40);normal.read(pose.data(),std::streamsize(pose.size()));if(!normal)throw std::runtime_error("benchmark normal query truncated");const auto*proc=namedAttribute(graph,graph.elements.at(instance.id),"evaluateProceduralBones");if(!proc||proc->type!=4||proc->values.size()!=1||proc->values[0].wire.size()!=1)throw std::runtime_error("benchmark procedural setting schema");const std::array<uint32_t,3>wire{h[0],h[1],3u|(proc->values[0].wire[0]?4u:0u)};output.write(reinterpret_cast<const char*>(wire.data()),12);output.write(pose.data(),std::streamsize(pose.size()));
                for(const auto&b:snapshot){const std::array<uint32_t,2>bh{(b.present?1u:0u)|(b.transformTyped?2u:0u),uint32_t(b.references.size())};output.write(reinterpret_cast<const char*>(bh.data()),8);for(const auto&r:b.references){const uint32_t flags=(r.attributePresent?1u:0u)|(r.ownerPresent?2u:0u)|(r.ownerDagTyped?4u:0u)|(r.nameMatches?8u:0u)|(r.sameFile?16u:0u)|(r.overridesDisabled?32u:0u)|(r.overridePresent?64u:0u)|(r.overrideDagTyped?128u:0u);output.write(reinterpret_cast<const char*>(&flags),4);output.write(reinterpret_cast<const char*>(r.world.data()),48);}}
            }
            for(size_t i=0;i<array->values.size();++i){const auto id=array->values[i].reference;if(id>=graph.elements.size())continue;++bones;size_t matches=0;
                for(uint32_t owner=0;owner<graph.elements.size();++owner){const auto&e=graph.elements[owner];const auto&type=graph.strings.at(e.type);if(type!="DmeDag"&&type!="DmeGameModel")continue;
                    const auto*t=namedAttribute(graph,e,"transform");if(!t||t->type!=1||t->values.size()!=1||t->values[0].reference!=id)continue;++matches;++references;
                    const auto op=hierarchySingleReference(graph,e,"overrideParent");const bool enabled=!hierarchy.overridesDisabled(owner)&&op!=UINT32_MAX;
                    if(enabled){++overrides;const auto w=hierarchy.world(owner);std::cout<<"override tick="<<tick<<" model="<<instance.id<<" index="<<i<<" transform="<<id<<" dag="<<owner<<" parent="<<op<<" world="<<w[0][3]<<','<<w[1][3]<<','<<w[2][3]<<'\n';}
                }if(matches>1)++ambiguous;
            }
        }
        std::cout<<"summary tick="<<tick<<" bones="<<bones<<" referencing_dags="<<references<<" enabled_overrides="<<overrides<<" ambiguous="<<ambiguous<<'\n';
    }if(argc==4){if(normal.peek()!=std::char_traits<char>::eof())throw std::runtime_error("benchmark normal query trailing data");output.flush();if(!output)throw std::runtime_error("precompute export failed");}std::cout<<"PASS benchmark_bone_reference_frames=2\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
