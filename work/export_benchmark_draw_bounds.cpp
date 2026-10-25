#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "live_session_scene_candidate.h"
#include "session_render_root_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "read_binary_asset_candidate.h"
#include "session_ifm_bone_inputs_candidate.h"
#include "ifm_virtual_studio_bounds_candidate.h"
#include "ifm_draw_frames_candidate.h"
#include "dag_world_fixture.h"
#include <iostream>
#include <iomanip>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 export required");if(argc!=5||std::filesystem::exists(argv[3])||std::filesystem::exists(argv[4]))throw std::runtime_error("session gameinfo fresh-bounds fresh-palettes required");
    auto graph=loadGraph(argv[1]);const auto root=sessionRenderRootCandidate(graph);RetainedShotTimelineCandidate timeline(std::move(graph),root);
    const auto bytes=readBinaryAssetCandidate(argv[2]);const std::string text(bytes.begin(),bytes.end());const auto content=std::filesystem::path(argv[2]).parent_path();const auto mounts=gameinfoMountsCandidate(text,content,content.parent_path());
    auto load=[&](const std::string&name){for(const auto&mount:mounts)if(std::filesystem::is_regular_file(mount/name)){std::cout<<"asset_path="<<(mount/name).string()<<'\n';return readBinaryAssetCandidate(mount/name);}throw std::runtime_error("benchmark bounds asset missing");};
    std::ofstream out(argv[3],std::ios::binary),paletteOut(argv[4],std::ios::binary);if(!out||!paletteOut)throw std::runtime_error("bounds export output");size_t cases=0,models=0;
    std::cout<<std::setprecision(9);
    for(int32_t tick:{50000,50417}){const auto step=evaluateLiveSessionSceneCandidate(timeline,tick,1280,720);const auto&g=timeline.graph();const auto binding=readFilmSceneRenderBindingCandidate(g,step.scene.shot);GraphTransformHierarchy hierarchy(g,binding.scene);
        for(const auto&instance:step.scene.models){if(instance.placement.asset[0]=='*')continue;const uint32_t model=instance.id;const auto&e=g.elements.at(model);
            const auto*children=namedAttribute(g,e,"children"),*sequence=namedAttribute(g,e,"sequence"),*compute=namedAttribute(g,e,"computeBounds");
            if(!children||children->type!=15||children->values.empty()||!sequence||sequence->type!=2||sequence->values.size()!=1||!compute||compute->type!=4||compute->values.size()!=1||compute->values[0].wire.size()!=1)throw std::runtime_error("benchmark bounds model schema");
            std::vector<uint32_t>rawChildren;for(const auto&v:children->values){if(v.reference>=g.elements.size()||g.strings.at(g.elements[v.reference].type)!="DmeDag")throw std::runtime_error("benchmark raw child type/order unsupported");rawChildren.push_back(v.reference);}
            const auto mdl=load(instance.placement.asset);std::vector<SourceMdlSequenceMetadataCandidate>groups{readSourceMdlSequenceMetadataCandidate(mdl)};
            if(groups[0].includes.size()>1)throw std::runtime_error("benchmark include order unsupported");if(!groups[0].includes.empty()){const auto name=groups[0].includes[0];groups.push_back(readSourceMdlSequenceMetadataCandidate(load(name)));if(!groups[1].includes.empty())throw std::runtime_error("benchmark recursive include order unsupported");}
            const auto poses=sessionIfmBoneInputsCandidate(g,model,mdl,[&](uint32_t id){return g.strings.at(g.elements.at(id).type)=="DmeTransform";});const auto boxes=readIfmMdlHitboxesCandidate(mdl);
            for(const auto&b:poses)if(b.flags&4)throw std::runtime_error("benchmark procedural bone kernel needs separate provider");
            const auto modelWorld=hierarchy.world(model);const auto decomposition=ifmDecomposeFrameCandidate(modelWorld);float localScale=1;
            const auto transform=hierarchySingleReference(g,e,"transform");if(transform!=UINT32_MAX){const auto*s=namedAttribute(g,g.elements.at(transform),"scale");if(s){if(s->type!=3||s->values.size()!=1||s->values[0].wire.size()!=4)throw std::runtime_error("benchmark local scale conversion unsupported");localScale=wireFloat(s->values[0]);}}
            auto boneRoot=ifmBoundsAngleMatrixCandidate(decomposition.angles,decomposition.position);if(localScale!=1)for(auto&r:boneRoot)for(unsigned c=0;c<3;++c)r[c]*=localScale;checkIfmMatrixFiniteCandidate(boneRoot);
            const uint32_t count=uint32_t(poses.size()),mask=0x7ff00;const auto palette=ifmBoneWorldPaletteCandidate(poses,boneRoot,mask);std::vector<RecoveredMatrix3x4>matrices(count);for(uint32_t i=0;i<count;++i){for(auto&r:matrices[i])r.fill(12345.25f);if(palette[i])matrices[i]=*palette[i];}
            for(const auto&b:boxes.firstSet)if(!palette.at(size_t(b.bone)))throw std::runtime_error("benchmark missing hitbox bone");
            paletteOut.write(reinterpret_cast<const char*>(&count),4);paletteOut.write(reinterpret_cast<const char*>(&mask),4);paletteOut.write(reinterpret_cast<const char*>(boneRoot.data()),48);paletteOut.write(reinterpret_cast<const char*>(poses.data()),std::streamsize(count*40));
            std::set<uint32_t>closure;std::vector<uint32_t>pending=rawChildren;pending.push_back(model);
            while(!pending.empty()){const auto id=pending.back();pending.pop_back();if(!closure.insert(id).second)continue;if(closure.size()>64)throw std::runtime_error("draw bounds DAG closure limit");const auto&type=g.strings.at(g.elements.at(id).type);if(type!="DmeDag"&&type!="DmeGameModel")throw std::runtime_error("draw bounds DAG ancestry unsupported");const auto ordinary=hierarchy.ordinaryParents.find(id);if(ordinary!=hierarchy.ordinaryParents.end())pending.push_back(ordinary->second);const auto over=hierarchySingleReference(g,g.elements.at(id),"overrideParent");if(over!=UINT32_MAX)pending.push_back(over);}
            std::vector<uint32_t>order{model};for(auto id:rawChildren)if(std::find(order.begin(),order.end(),id)==order.end())order.push_back(id);for(auto id:closure)if(std::find(order.begin(),order.end(),id)==order.end())order.push_back(id);
            std::map<uint32_t,uint32_t>mapping;for(auto id:order)mapping.emplace(id,uint32_t(mapping.size()));auto remap=[&](uint32_t id){return id==UINT32_MAX?id:mapping.at(id);};std::vector<DagWorldFixtureNode>nodes(order.size());
            for(auto id:order){auto&n=nodes[mapping.at(id)];n.flags=0;const auto&dag=g.elements.at(id);const auto ordinary=hierarchy.ordinaryParents.find(id);n.parent=ordinary==hierarchy.ordinaryParents.end()?UINT32_MAX:remap(ordinary->second);n.overrideParent=remap(hierarchySingleReference(g,dag,"overrideParent"));
                auto flag=[&](const char*name,uint32_t present,uint32_t truth){const auto*a=namedAttribute(g,dag,name);if(!a)return;if(a->type!=4||a->values.size()!=1||a->values[0].wire.size()!=1)throw std::runtime_error("DAG bool schema");n.flags|=present;if(a->values[0].wire[0])n.flags|=truth;};flag("disableOverride",0,1);flag("overridePos",2,4);flag("overrideRot",8,16);
                const auto t=hierarchySingleReference(g,dag,"transform");if(t!=UINT32_MAX){n.flags|=32;const auto&tr=g.elements.at(t);if(g.strings.at(tr.type)!="DmeTransform")throw std::runtime_error("DAG transform type");auto copy=[&](const char*name,uint8_t type,void*dest,size_t size){const auto*a=namedAttribute(g,tr,name);if(!a||a->type!=type||a->values.size()!=1||a->values[0].wire.size()!=size)throw std::runtime_error("DAG transform schema");std::memcpy(dest,a->values[0].wire.data(),size);};copy("position",10,n.position.data(),12);copy("orientation",13,n.orientation.data(),16);if(namedAttribute(g,tr,"scale")){copy("scale",3,&n.scale,4);n.flags|=64;}}
            }
            validateDagWorldFixture(nodes);const auto normalized=dagWorldFixtureGraph(nodes);GraphTransformHierarchy norm(normalized);
            for(auto id:order){const auto a=hierarchy.world(id),b=norm.world(mapping.at(id));if(std::memcmp(a.data(),b.data(),48)||remap(hierarchy.parent(id,true))!=norm.parent(mapping.at(id),true)||remap(hierarchy.parent(id,false))!=norm.parent(mapping.at(id),false))throw std::runtime_error("bounds DAG normalization changed frames/parents");}
            std::vector<uint32_t>normalizedChildren;for(uint32_t i=0;i<nodes.size();++i)if(nodes[i].parent==0)normalizedChildren.push_back(order[i]);if(normalizedChildren!=rawChildren)throw std::runtime_error("bounds normalization changed raw child order/count");
            for(auto cache:{SourceSequenceNameCacheModeCandidate::Disabled,SourceSequenceNameCacheModeCandidate::Enabled}){const auto studio=ifmVirtualStudioBoundsCandidate(mdl,groups,cache);
                for(int32_t mode:{0,1})for(uint32_t renderFlags:{0u,0x20u}){const uint32_t flags=1|2|4|16|64|128|256|(compute->values[0].wire[0]?8:0);const int32_t seq=std::max(0,wireInt(sequence->values[0]));
                    const std::array<uint32_t,11>header{flags,3,uint32_t(seq),uint32_t(mode),renderFlags,0,uint32_t(nodes.size()),uint32_t(studio.sequences.size()),count,uint32_t(boxes.firstSet.size()),uint32_t(tick)};
                    std::array<float,49>values{};std::memcpy(values.data()+6,studio.hull.data(),24);std::memcpy(values.data()+12,studio.view.data(),24);std::memcpy(values.data()+30,modelWorld.data(),48);if(!poses.empty()){std::memcpy(values.data()+42,poses[0].position.data(),12);std::memcpy(values.data()+45,poses[0].orientation.data(),16);}
                    out.write(reinterpret_cast<const char*>(header.data()),44);out.write(reinterpret_cast<const char*>(values.data()),196);out.write(reinterpret_cast<const char*>(nodes.data()),std::streamsize(nodes.size()*44));out.write(reinterpret_cast<const char*>(studio.sequences.data()),std::streamsize(studio.sequences.size()*24));out.write(reinterpret_cast<const char*>(matrices.data()),std::streamsize(matrices.size()*48));out.write(reinterpret_cast<const char*>(boxes.firstSet.data()),std::streamsize(boxes.firstSet.size()*28));
                    std::cout<<"binding tick="<<tick<<" model="<<model<<" child0="<<rawChildren[0]<<" sequence="<<seq<<" cache="<<(cache==SourceSequenceNameCacheModeCandidate::Enabled)<<" mode="<<mode<<" render_flags="<<renderFlags<<" compute="<<int(compute->values[0].wire[0])<<" bones="<<count<<" boxes="<<boxes.firstSet.size()<<" root_position="<<decomposition.position[0]<<','<<decomposition.position[1]<<','<<decomposition.position[2]<<'\n';++cases;
                }
            }++models;
        }
    }out.flush();paletteOut.flush();if(!out||!paletteOut||cases!=32||models!=4)throw std::runtime_error("benchmark draw bounds export coverage");std::cout<<"PASS cases="<<cases<<" models="<<models<<" palette_binding=normal_no_precomputed native_runtime_state=UNVERIFIED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
