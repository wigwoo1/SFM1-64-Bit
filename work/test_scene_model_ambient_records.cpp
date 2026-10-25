#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "retained_session_scene_candidate.h"
#include "session_render_root_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "scene_model_ambient_records_candidate.h"
#include <iostream>
#include <limits>

int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 required");
    auto require=[](bool value,const char*message){if(!value)throw std::runtime_error(message);};
    struct Model {struct {bool visible=true;}model;std::vector<GeometryDeformedVertexCandidate>vertices;};
    std::vector<Model>models(3);for(auto&m:models)m.vertices.resize(2);
    models[0].vertices[0].normal={2,0,0};models[0].vertices[1].normal={0,-3,0};
    models[1].vertices[0].normal={0,0,0};models[2].model.visible=false;
    models[2].vertices[0].normal={INFINITY,0,0};
    GenericAmbientCubeCandidate cube{};for(unsigned face=0;face<6;++face)cube[face]={float(face+1),float(face+11),float(face+21)};
    std::vector<std::optional<GenericAmbientCubeCandidate>>cubes(3);cubes[0]=cube;cubes[2]=cube;
    const auto savedModels=models;const auto savedCubes=cubes;
    auto result=prepareSceneModelAmbientRecordsCandidate(models,cubes);
    require(result.records.size()==2&&result.addresses.size()==2,"visible explicit model coverage");
    require(result.addresses[0].model==0&&result.addresses[1].vertex==1,"model/vertex identity");
    require(result.records[0].worldNormal==std::array<float,4>{2,0,0,0},"raw world normal changed");
    require(sceneModelAmbientRecordReferenceCandidate(result.records[0])==cube[0]&&sceneModelAmbientRecordReferenceCandidate(result.records[1])==cube[3],"normalization/sign/face handoff");
    auto snapshot=result.records;std::fill(cubes[0]->at(0).begin(),cubes[0]->at(0).end(),99.f);models[0].vertices[0].normal={0,0,1};
    require(std::memcmp(snapshot.data(),result.records.data(),snapshot.size()*sizeof(snapshot[0]))==0,"records borrow mutable inputs");
    models=savedModels;cubes=savedCubes;
    unsigned rejections=0;
    auto reject=[&](const auto&m,const auto&c){try{(void)prepareSceneModelAmbientRecordsCandidate(m,c);}catch(const std::invalid_argument&){++rejections;return;}throw std::runtime_error("invalid ambient handoff accepted");};
    auto badCubes=cubes;badCubes.pop_back();reject(models,badCubes);
    for(float bad:{INFINITY,-INFINITY,std::numeric_limits<float>::quiet_NaN()}){
        for(unsigned axis=0;axis<3;++axis){auto invalid=models;invalid[0].vertices[1].normal[axis]=bad;reject(invalid,cubes);}
        for(unsigned face=0;face<6;++face)for(unsigned channel=0;channel<3;++channel){auto invalid=cubes;(*invalid[0])[face][channel]=bad;reject(models,invalid);}
    }
    {auto invalid=models;invalid[0].vertices[0].normal={0,0,0};reject(invalid,cubes);}
    {auto invalid=models;invalid[0].vertices[0].normal={std::numeric_limits<float>::max(),0,0};reject(invalid,cubes);}
    require(std::memcmp(snapshot.data(),result.records.data(),snapshot.size()*sizeof(snapshot[0]))==0,"failed preparation changed prior records");
    require(argc==4,"usage: test session.dmx gameinfo.txt fresh-records.bin");
    require(!std::filesystem::exists(argv[3]),"fresh workspace output required");
    auto graph=loadGraph(argv[1]);const auto root=sessionRenderRootCandidate(graph);
    const auto bytes=readBinaryAssetCandidate(argv[2]);const std::string text(bytes.begin(),bytes.end());
    const auto content=std::filesystem::path(argv[2]).parent_path();
    auto mounts=gameinfoMountsCandidate(text,content,content.parent_path());
    RetainedSessionSceneCandidate scene(std::move(graph),root,mounts,4,true,true);
    std::vector<SceneModelAmbientRecordCandidate>all,first;
    size_t frameCount=0,firstVertices=0,secondChanged=0;
    scene.addSceneDecorationCandidate([&](const Graph&,const SessionRenderSceneCandidate&,const auto&geometry,const auto&,PreparedSessionSceneCandidate&prepared){
        std::vector<std::optional<GenericAmbientCubeCandidate>>explicitCubes(geometry.size());
        size_t eligible=0;
        for(size_t m=0;m<geometry.size();++m){
            const auto&name=geometry[m].model.placement.asset;
            if(name.empty()||name[0]=='*')continue; // explicit test selection excludes inline brush
            ++eligible;auto selected=cube;
            for(unsigned face=0;face<6;++face)for(unsigned c=0;c<3;++c)selected[face][c]=float(1+m+face+c)/16.0f;
            explicitCubes[m]=selected;
        }
        require(eligible==2,"benchmark actor selection changed");
        const auto before=prepared.upload.vertices;const auto beforeIndices=prepared.upload.indices;
        const auto packed=prepareSceneModelAmbientRecordsCandidate(geometry,explicitCubes);
        require(packed.records.size()==packed.addresses.size()&&!packed.records.empty(),"benchmark ambient records missing");
        for(size_t i=0;i<packed.records.size();++i){const auto&a=packed.addresses[i];const auto&source=geometry.at(a.model).vertices.at(a.vertex);
            require(std::memcmp(source.normal.data(),packed.records[i].worldNormal.data(),12)==0,"world-normal identity lost");
            for(unsigned face=0;face<6;++face)require(std::memcmp((*explicitCubes.at(a.model))[face].data(),packed.records[i].cube[face].data(),12)==0,"model cube mixed");
        }
        require(before.size()==prepared.upload.vertices.size()&&std::memcmp(before.data(),prepared.upload.vertices.data(),before.size()*sizeof(before[0]))==0&&beforeIndices==prepared.upload.indices,"geometry or BSP light-color channel changed");
        if(!frameCount){first=packed.records;firstVertices=before.size();}else{
            require(first.size()==packed.records.size(),"frame record addressing changed");
            for(size_t i=0;i<first.size();++i)if(std::memcmp(first[i].worldNormal.data(),packed.records[i].worldNormal.data(),12))++secondChanged;
        }
        all.insert(all.end(),packed.records.begin(),packed.records.end());++frameCount;
    });
    auto a=scene.sample(50000,1280,720);auto b=scene.sample(50417,1280,720);
    require(frameCount==2&&firstVertices==52497&&a.upload.indices.size()==110466&&b.upload.indices.size()==110466&&secondChanged>0,"benchmark geometry/frame progression");
    std::ofstream output(argv[3],std::ios::binary);output.write(reinterpret_cast<const char*>(all.data()),std::streamsize(all.size()*sizeof(all[0])));require(bool(output),"record output failed");
    std::cout<<"PASS ambient_handoff_rejections="<<rejections<<" snapshot_owned=1 original_geometry_channels_unchanged=1 benchmark_frames="<<frameCount<<" benchmark_records="<<all.size()<<" animated_normals_changed="<<secondChanged<<" source_vertices="<<firstVertices<<" source_indices=110466 native_lighting_parity=unverified\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
