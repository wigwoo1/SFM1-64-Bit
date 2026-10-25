#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "live_session_scene_candidate.h"
#include "session_render_root_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "read_binary_asset_candidate.h"
#include "session_ifm_bone_inputs_candidate.h"
#include "session_ifm_bone_references_candidate.h"
#include "ifm_setup_bones_candidate.h"
#include "studio_skin_palette_candidate.h"
#include "scene_geometry_preparation_candidate.h"
#include <iostream>
#include <iomanip>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 studio audit");if(argc!=5||std::filesystem::exists(argv[3])||std::filesystem::exists(argv[4]))throw std::runtime_error("session gameinfo fresh-palette-query fresh-geometry-records required");
    auto graph=loadGraph(argv[1]);const auto root=sessionRenderRootCandidate(graph);RetainedShotTimelineCandidate timeline(std::move(graph),root);
    const auto info=readBinaryAssetCandidate(argv[2]);const auto content=std::filesystem::path(argv[2]).parent_path();const auto mounts=gameinfoMountsCandidate(std::string(info.begin(),info.end()),content,content.parent_path());
    auto load=[&](const std::string&name){for(const auto&m:mounts)if(std::filesystem::is_regular_file(m/name)){std::cout<<"asset_path="<<(m/name).string()<<'\n';return readBinaryAssetCandidate(m/name);}throw std::runtime_error("studio audit mounted asset missing");};
    std::ofstream query(argv[3],std::ios::binary),records(argv[4],std::ios::binary);if(!query||!records)throw std::runtime_error("studio audit output");size_t cases=0,vertices=0;GeometryDeformationWorkerPoolCandidate one(1),four(4);std::cout<<std::setprecision(9);
    for(int32_t tick:{50000,50417}){const auto step=evaluateLiveSessionSceneCandidate(timeline,tick,1280,720);const auto&g=timeline.graph();const auto binding=readFilmSceneRenderBindingCandidate(g,step.scene.shot);GraphTransformHierarchy hierarchy(g,binding.scene);
        for(const auto&instance:step.scene.models){if(instance.placement.asset[0]=='*')continue;const auto mdl=load(instance.placement.asset);auto path=std::filesystem::path(instance.placement.asset);path.replace_extension();const auto vvd=load(path.generic_string()+".vvd"),vtx=load(path.generic_string()+".dx90.vtx");const auto asset=readSfmModelGeometryCandidate(mdl,vvd,vtx);const auto metadata=readStudioSkinPaletteMetadataCandidate(mdl);if(metadata.staticModel)throw std::runtime_error("benchmark static actor unsupported");
            auto word=[&](size_t at){if(at>mdl.size()||mdl.size()-at<4)throw std::runtime_error("studio audit MDL span");uint32_t v;std::memcpy(&v,mdl.data()+at,4);return v;};const auto hdr2=word(400);if(hdr2&&word(size_t(hdr2)+24))throw std::runtime_error("studio audit bone flex drivers unsupported");
            const auto poses=sessionIfmBoneInputsCandidate(g,instance.id,mdl,[&](uint32_t id){return g.strings.at(g.elements.at(id).type)=="DmeTransform";});for(const auto&p:poses)if(p.flags&4)throw std::runtime_error("studio audit procedural bones unsupported");
            const auto refs=sessionIfmBoneReferencesCandidate(g,instance.id,uint32_t(poses.size()),[&](uint32_t id){const auto&t=g.strings.at(g.elements.at(id).type);return t=="DmeDag"||t=="DmeGameModel";},[&](uint32_t id){return g.strings.at(g.elements.at(id).type)=="DmeTransform";},[&](uint32_t id){return hierarchy.world(id);});
            const auto pre=ifmBonePrecomputeCandidate(true,true,refs);for(const auto&p:pre.palette)if(p)throw std::runtime_error("benchmark precomputed fixture changed");
            float scale=1;const auto tr=hierarchySingleReference(g,g.elements.at(instance.id),"transform");if(tr!=UINT32_MAX){const auto*s=namedAttribute(g,g.elements.at(tr),"scale");if(s){if(s->type!=3||s->values.size()!=1||s->values[0].wire.size()!=4)throw std::runtime_error("studio audit root scale");scale=wireFloat(s->values[0]);}}
            const auto boneRoot=ifmSetupBoneRootCandidate(hierarchy.world(instance.id),scale);const auto prepared=ifmBoneWorldPaletteCandidate(poses,boneRoot,0x7ff00,pre.palette);std::vector<RecoveredMatrix3x4>world(poses.size()),initial(poses.size());for(auto&m:world)for(auto&r:m)r.fill(12345.25f);for(auto&m:initial)for(auto&r:m)r.fill(-23456.5f);for(size_t i=0;i<world.size();++i)if(prepared[i])world[i]=*prepared[i];
            const auto skin=studioSkinPaletteCandidate(metadata,world,0x7ff00,initial);std::vector<GeometryMatrixCandidate>palette(skin.size());for(size_t i=0;i<skin.size();++i)std::memcpy(palette[i].data(),skin[i].data(),48);
            for(const auto&v:asset.geometry.vertices){if(!v[15]||v[15]>3)throw std::runtime_error("studio audit vertex influence count");for(uint32_t i=0;i<v[15];++i){const auto bone=v[12+i];if(bone>=prepared.size()||!prepared[bone]||!(metadata.flags[bone]&0x7ff00))throw std::runtime_error("studio audit used vertex bone unprepared");}}
            // Compare the same existing CPU skinning policy on two input
            // palettes. New inputs are already WORLD-space: no second placement.
            const auto corrected=one.deform(asset.geometry,palette),parallel=four.deform(asset.geometry,palette);
            const auto legacyLocals=readSessionBonePoseCandidate(g,instance.id,asset.bones,true,true);const auto legacyPalette=evaluateGeometryPosePaletteCandidate(asset.bones,legacyLocals,nullptr,instance.evaluateProcedural);const auto legacy=one.deform(asset.geometry,legacyPalette,&instance.placement.matrix);
            size_t changedPosition=0,changedNormal=0;double maxPosition=0,maxNormal=0;
            for(size_t i=0;i<corrected.size();++i){const auto&a=corrected[i],&b=parallel[i],&old=legacy[i];if(a.position!=b.position||a.normal!=b.normal||a.tangent!=b.tangent||a.uv!=b.uv||a.hasTangent!=b.hasTangent)throw std::runtime_error("studio audit worker mismatch");changedPosition+=std::memcmp(a.position.data(),old.position.data(),12)!=0;changedNormal+=std::memcmp(a.normal.data(),old.normal.data(),12)!=0;for(unsigned c=0;c<3;++c){maxPosition=std::max(maxPosition,std::abs(double(a.position[c])-old.position[c]));maxNormal=std::max(maxNormal,std::abs(double(a.normal[c])-old.normal[c]));}}
            const std::array<uint32_t,3>q{uint32_t(mdl.size()),uint32_t(world.size()),0x7ff00};query.write(reinterpret_cast<const char*>(q.data()),12);query.write(reinterpret_cast<const char*>(mdl.data()),std::streamsize(mdl.size()));query.write(reinterpret_cast<const char*>(world.data()),std::streamsize(world.size()*48));query.write(reinterpret_cast<const char*>(initial.data()),std::streamsize(initial.size()*48));
            const std::array<uint32_t,4>h{uint32_t(tick),instance.id,uint32_t(world.size()),uint32_t(corrected.size())};records.write(reinterpret_cast<const char*>(h.data()),16);records.write(reinterpret_cast<const char*>(world.data()),std::streamsize(world.size()*48));records.write(reinterpret_cast<const char*>(skin.data()),std::streamsize(skin.size()*48));
            for(size_t i=0;i<corrected.size();++i)for(const auto*v:{&corrected[i],&legacy[i]}){std::array<float,12>values{};std::copy(v->position.begin(),v->position.end(),values.begin());std::copy(v->normal.begin(),v->normal.end(),values.begin()+3);std::copy(v->uv.begin(),v->uv.end(),values.begin()+6);std::copy(v->tangent.begin(),v->tangent.end(),values.begin()+8);records.write(reinterpret_cast<const char*>(values.data()),48);}
            std::cout<<"binding tick="<<tick<<" model="<<instance.id<<" bones="<<world.size()<<" linear="<<metadata.linear<<" vertices="<<corrected.size()<<" changed_positions="<<changedPosition<<" max_position_delta="<<maxPosition<<" changed_normals="<<changedNormal<<" max_normal_delta="<<maxNormal<<'\n';++cases;vertices+=corrected.size();
        }
    }query.flush();records.flush();if(!query||!records||cases!=4)throw std::runtime_error("studio audit coverage");std::cout<<"PASS studio_skin_benchmark_cases="<<cases<<" vertices="<<vertices<<" workers=1,4 CPU_policy=legacy_unflexed native_shader_vertex_parity=UNVERIFIED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
