#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "live_session_scene_candidate.h"
#include "session_render_root_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "read_binary_asset_candidate.h"
#include "ifm_draw_frames_candidate.h"
#include "ifm_renderable_bounds_candidate.h"
#include "source_mdl_virtual_sequences_candidate.h"
#include <iostream>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 required");if(argc!=4||std::filesystem::exists(argv[3]))throw std::runtime_error("session gameinfo fresh-queries required");
    auto g=loadGraph(argv[1]);const auto root=sessionRenderRootCandidate(g);RetainedShotTimelineCandidate timeline(std::move(g),root);
    const auto info=readBinaryAssetCandidate(argv[2]);const std::string infoText(info.begin(),info.end());const auto content=std::filesystem::path(argv[2]).parent_path();
    const auto mounts=gameinfoMountsCandidate(infoText,content,content.parent_path());
    auto load=[&](const std::string&name){for(const auto&mount:mounts)if(std::filesystem::is_regular_file(mount/name)){std::cout<<"metadata_path="<<(mount/name).string()<<'\n';return readSourceMdlSequenceMetadataCandidate(readBinaryAssetCandidate(mount/name));}throw std::runtime_error("sequence model missing");};
    std::ofstream out(argv[3],std::ios::binary);if(!out)throw std::runtime_error("sequence fixture output");size_t cases=0;
    for(int32_t tick:{50000,50417}){
        const auto step=evaluateLiveSessionSceneCandidate(timeline,tick,1280,720);const auto&graph=timeline.graph();
        const auto binding=readFilmSceneRenderBindingCandidate(graph,step.scene.shot);GraphTransformHierarchy hierarchy(graph,binding.scene);
        for(const auto&instance:step.scene.models){if(instance.placement.asset[0]=='*')continue;
            const auto&e=graph.elements.at(instance.id);std::cout<<"tick="<<tick<<" model="<<instance.id<<" asset="<<instance.placement.asset;
            for(const char*name:{"sequence","computeBounds","children"}){
                const auto*a=namedAttribute(graph,e,name);std::cout<<' '<<name<<'=';
                if(!a)std::cout<<"ABSENT";else{std::cout<<"type"<<unsigned(a->type)<<":";
                    if(a->type==15){for(const auto&v:a->values){std::cout<<v.reference;if(v.reference<graph.elements.size())std::cout<<'/'<<graph.strings.at(graph.elements[v.reference].type);std::cout<<',';}}
                    else if(a->type==2&&a->values.size()==1)std::cout<<wireInt(a->values[0]);
                    else if(a->type==4&&a->values.size()==1&&a->values[0].wire.size()==1)std::cout<<int(a->values[0].wire[0]);
                }
            }
            const auto world=hierarchy.world(instance.id);std::cout<<" world=";for(const auto&r:world)for(float v:r)std::cout<<v<<',';std::cout<<'\n';
            std::vector<SourceMdlSequenceMetadataCandidate>groups{load(instance.placement.asset)};
            // This benchmark closure has at most one included model and no
            // recursive includes. Other loading/dedup orders need a separate
            // native contract; do not guess a generic group allocator here.
            if(groups[0].includes.size()>1)throw std::runtime_error("benchmark include order needs verification");
            if(!groups[0].includes.empty()){const auto name=groups[0].includes[0];groups.push_back(load(name));if(!groups[1].includes.empty())throw std::runtime_error("nested benchmark include order unsupported");}
            const auto*a=namedAttribute(graph,e,"sequence");if(!a||a->type!=2||a->values.size()!=1)throw std::runtime_error("benchmark sequence schema");const int32_t sequence=std::max(0,wireInt(a->values[0]));
            const auto table=sourceVirtualSequencesCandidate(groups);const auto selected=sourceVirtualSequenceCandidate(table,sequence);const auto&local=groups.at(selected.group).sequences.at(selected.index);
            std::cout<<"selection tick="<<tick<<" model="<<instance.id<<" session_sequence="<<wireInt(a->values[0])<<" synchronized_sequence="<<sequence<<" groups="<<groups.size()<<" virtual_sequences="<<table.entries.size()<<" selected_group="<<selected.group<<" selected_local="<<selected.index<<" label="<<local.label<<" bounds=";for(float v:local.bounds)std::cout<<v<<',';std::cout<<'\n';
            auto word=[&](uint32_t v){out.write(reinterpret_cast<const char*>(&v),4);};word(uint32_t(groups.size()));
            for(const auto&group:groups){word(uint32_t(group.sequences.size()));for(const auto&s:group.sequences){word(s.flags);word(s.activity);word(uint32_t(s.label.size()));out.write(s.label.data(),std::streamsize(s.label.size()));}}
            ++cases;
        }
    }out.flush();if(!out||cases!=4)throw std::runtime_error("benchmark sequence coverage/output");std::cout<<"PASS benchmark_cases="<<cases<<" native_live_state=UNVERIFIED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
