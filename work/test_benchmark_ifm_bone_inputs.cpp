#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "live_session_scene_candidate.h"
#include "session_render_root_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "read_binary_asset_candidate.h"
#include "session_ifm_bone_inputs_candidate.h"
#include "ifm_renderable_bounds_candidate.h"
#include <iostream>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 required");
    if(argc!=4||std::filesystem::exists(argv[3]))throw std::runtime_error("session gameinfo fresh-queries required");
    auto g=loadGraph(argv[1]);auto root=sessionRenderRootCandidate(g);RetainedShotTimelineCandidate timeline(std::move(g),root);
    auto info=readBinaryAssetCandidate(argv[2]);std::string text(info.begin(),info.end());auto content=std::filesystem::path(argv[2]).parent_path();auto mounts=gameinfoMountsCandidate(text,content,content.parent_path());
    std::ofstream out(argv[3],std::ios::binary);if(!out)throw std::runtime_error("queries create");size_t cases=0,boneTotal=0;
    for(int32_t tick:{50000,50417}){auto step=evaluateLiveSessionSceneCandidate(timeline,tick,1280,720);const auto&graph=timeline.graph();
        for(const auto&instance:step.scene.models){if(instance.placement.asset[0]=='*')continue;
            std::filesystem::path path;for(const auto&mount:mounts)if(std::filesystem::is_regular_file(mount/instance.placement.asset)){path=mount/instance.placement.asset;break;}if(path.empty())throw std::runtime_error("MDL missing");
            const auto mdl=readBinaryAssetCandidate(path);const auto inputs=sessionIfmBoneInputsCandidate(graph,instance.id,mdl,[&](uint32_t id){return graph.strings.at(graph.elements.at(id).type)=="DmeTransform";});
            const auto boxes=readIfmMdlHitboxesCandidate(mdl);const uint32_t count=uint32_t(inputs.size()),mask=0x7ff00;
            RecoveredMatrix3x4 modelWorld{};std::memcpy(modelWorld.data(),instance.placement.matrix.rows.data(),48);
            const auto palette=ifmBoneWorldPaletteCandidate(inputs,modelWorld,mask);for(const auto&box:boxes.firstSet)if(!palette.at(size_t(box.bone)))throw std::runtime_error("benchmark hitbox lacks world bone");
            out.write(reinterpret_cast<const char*>(&count),4);out.write(reinterpret_cast<const char*>(&mask),4);out.write(reinterpret_cast<const char*>(modelWorld.data()),48);out.write(reinterpret_cast<const char*>(inputs.data()),std::streamsize(count*40));
            const auto*a=namedAttribute(graph,graph.elements[instance.id],"bones");const auto*compute=namedAttribute(graph,graph.elements[instance.id],"computeBounds");
            if(compute&&(compute->type!=4||compute->values.size()!=1||compute->values[0].wire.size()!=1))throw std::runtime_error("computeBounds schema");
            auto integer=[&](size_t at){if(at>mdl.size()||mdl.size()-at<4)throw std::runtime_error("benchmark MDL metadata span");int32_t v;std::memcpy(&v,mdl.data()+at,4);return v;};
            const auto extra=integer(0x190);if(extra<0)throw std::runtime_error("header2 offset");const auto illuminationIndex=extra?integer(size_t(extra)+8):0;
            size_t proceduralFlags=0;for(const auto&b:inputs)proceduralFlags+=bool(b.flags&4);
            std::cout<<"binding tick="<<tick<<" model="<<instance.id<<" asset="<<instance.placement.asset<<" saved_bones="<<a->values.size()<<" mdl_bones="<<count<<" hitboxes="<<boxes.firstSet.size()<<" evaluate_procedural="<<instance.evaluateProcedural<<" procedural_flags="<<proceduralFlags<<" compute_bounds="<<(compute?int(compute->values[0].wire[0]):1)<<" illumination_index="<<illuminationIndex<<" local_attachments="<<integer(0xf0)<<" path="<<path.string()<<'\n';
            ++cases;boneTotal+=count;
        }
    }
    out.flush();if(!out||cases!=4)throw std::runtime_error("benchmark coverage/output");std::cout<<"PASS benchmark_cases="<<cases<<" bones="<<boneTotal<<" input_order=session_array native_root_frame=unverified procedural_kernel_test=disabled\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
