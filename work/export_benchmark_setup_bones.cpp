#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "live_session_scene_candidate.h"
#include "session_render_root_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "read_binary_asset_candidate.h"
#include "session_ifm_bone_inputs_candidate.h"
#include "session_ifm_bone_references_candidate.h"
#include <iostream>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 benchmark export");if(argc!=4||std::filesystem::exists(argv[3]))throw std::runtime_error("session gameinfo fresh-query required");
    auto g=loadGraph(argv[1]);const auto root=sessionRenderRootCandidate(g);RetainedShotTimelineCandidate timeline(std::move(g),root);
    const auto bytes=readBinaryAssetCandidate(argv[2]);const std::string text(bytes.begin(),bytes.end());const auto content=std::filesystem::path(argv[2]).parent_path();const auto mounts=gameinfoMountsCandidate(text,content,content.parent_path());
    std::ofstream out(argv[3],std::ios::binary);if(!out)throw std::runtime_error("setup benchmark output");size_t cases=0;
    for(int32_t tick:{50000,50417}){const auto step=evaluateLiveSessionSceneCandidate(timeline,tick,1280,720);const auto&graph=timeline.graph();const auto binding=readFilmSceneRenderBindingCandidate(graph,step.scene.shot);GraphTransformHierarchy hierarchy(graph,binding.scene);
        for(const auto&instance:step.scene.models){if(instance.placement.asset[0]=='*')continue;std::vector<uint8_t>mdl;for(const auto&mount:mounts)if(std::filesystem::is_regular_file(mount/instance.placement.asset)){std::cout<<"model_path="<<(mount/instance.placement.asset).string()<<'\n';mdl=readBinaryAssetCandidate(mount/instance.placement.asset);break;}if(mdl.empty())throw std::runtime_error("setup model missing");
            auto word=[&](size_t offset){if(offset>mdl.size()||mdl.size()-offset<4)throw std::runtime_error("setup MDL field");uint32_t v;std::memcpy(&v,mdl.data()+offset,4);return v;};const auto header2=word(0x190);if(header2&&word(size_t(header2)+0x18))throw std::runtime_error("setup bone-flex-driver provider required");
            const auto model=instance.id;const auto&e=graph.elements.at(model);const auto poses=sessionIfmBoneInputsCandidate(graph,model,mdl,[&](uint32_t id){return graph.strings.at(graph.elements.at(id).type)=="DmeTransform";});
            for(const auto&p:poses)if(p.flags&4)throw std::runtime_error("setup procedural provider required");
            const auto refs=sessionIfmBoneReferencesCandidate(graph,model,uint32_t(poses.size()),[&](uint32_t id){const auto&t=graph.strings.at(graph.elements.at(id).type);return t=="DmeDag"||t=="DmeGameModel";},[&](uint32_t id){return graph.strings.at(graph.elements.at(id).type)=="DmeTransform";},[&](uint32_t id){return hierarchy.world(id);});
            for(const auto&p:ifmBonePrecomputeCandidate(true,true,refs).palette)if(p)throw std::runtime_error("setup benchmark precomputed native provider required");
            const auto*a=namedAttribute(graph,e,"bones");if(!a||a->values.size()!=poses.size())throw std::runtime_error("setup benchmark bone array mismatch");std::vector<uint32_t>typed;for(const auto&v:a->values){if(v.reference==UINT32_MAX)typed.push_back(0);else{if(v.reference>=graph.elements.size())throw std::runtime_error("setup external transform");const auto&b=graph.elements[v.reference];typed.push_back((graph.strings.at(b.type)=="DmeTransform"?3u:1u)|(namedAttribute(graph,b,"scale")?4u:0u));}}
            const auto*weights=namedAttribute(graph,e,"flexWeights");if(!weights||weights->type!=17||weights->values.size()>256)throw std::runtime_error("setup flex input schema");std::vector<float>flex;for(const auto&v:weights->values){if(v.wire.size()!=4)throw std::runtime_error("setup flex wire");flex.push_back(wireFloat(v));}
            float scale=1;uint32_t flags=1|2|4|8|64;const auto transform=hierarchySingleReference(graph,e,"transform");if(transform!=UINT32_MAX){const auto*s=namedAttribute(graph,graph.elements.at(transform),"scale");if(s){if(s->type!=3||s->values.size()!=1||s->values[0].wire.size()!=4)throw std::runtime_error("setup root scale schema");scale=wireFloat(s->values[0]);flags|=16;}}
            const auto*proc=namedAttribute(graph,e,"evaluateProceduralBones");if(!proc||proc->type!=4||proc->values.size()!=1||proc->values[0].wire.size()!=1)throw std::runtime_error("setup procedural setting");if(proc->values[0].wire[0])flags|=32;
            const uint32_t count=uint32_t(poses.size());const std::array<uint32_t,9>h{1,flags,count,count,count,4,uint32_t(flex.size()),0,uint32_t(tick)};const auto world=hierarchy.world(model);std::vector<RecoveredMatrix3x4>cache(256);for(auto&m:cache)for(auto&r:m)r.fill(12345.25f);
            out.write(reinterpret_cast<const char*>(h.data()),36);out.write(reinterpret_cast<const char*>(world.data()),48);out.write(reinterpret_cast<const char*>(&scale),4);out.write(reinterpret_cast<const char*>(poses.data()),count*40);out.write(reinterpret_cast<const char*>(typed.data()),count*4);out.write(reinterpret_cast<const char*>(flex.data()),std::streamsize(flex.size()*4));out.write(reinterpret_cast<const char*>(cache.data()),256*48);
            const std::array<std::array<uint32_t,4>,4>requests{{{256,0x100,0x40a00000,1},{256,0,0x7fc00001,1},{count-1,0xffffffff,0,1},{0x80000000,0,0xff800000,0}}};for(const auto&r:requests)out.write(reinterpret_cast<const char*>(r.data()),16);
            std::cout<<"binding tick="<<tick<<" model="<<model<<" bones="<<count<<" flex="<<flex.size()<<" drivers=0 precomputed=0\n";++cases;
        }
    }out.flush();if(!out||cases!=4)throw std::runtime_error("setup benchmark coverage");std::cout<<"PASS setup_benchmark_cases=4\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
