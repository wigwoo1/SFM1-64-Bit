#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "retained_session_scene_candidate.h"
#include "session_render_root_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "session_model_illumination_dag_candidate.h"
#include "source_new_record_leaf_ambient_candidate.h"
#include "source_bsp_light_cache_world_candidate.h"
#include "source_bsp_displacement_collision_candidate.h"
#include "scene_model_ambient_records_candidate.h"
#include <iomanip>
#include <iostream>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"Native x64 required");
    auto require=[](bool good,const char*message){if(!good)throw std::runtime_error(message);};
    require(argc==5,"usage: test session.dmx gameinfo.txt map.bsp fresh-records.bin");
    require(!std::filesystem::exists(argv[4]),"fresh workspace output required");
    auto bytes=readBinaryAssetCandidate(argv[3]);SourceBspBrushOnlyTraceCandidate world(bytes,std::vector<uint32_t>(32768));SourceBspDisplacementCollisionCandidate displacements(bytes);SourcePreparedWorldRayCandidate rays(world,displacements.meshes);SourceBspLightCacheWorldCandidate cache(bytes,world,rays);SourceBspLeafAmbientCandidate hdr(bytes,SourceBspAmbientEncodingCandidate::Hdr),ldr(bytes,SourceBspAmbientEncodingCandidate::Ldr);
    auto graph=loadGraph(argv[1]);const auto root=sessionRenderRootCandidate(graph);
    auto info=readBinaryAssetCandidate(argv[2]);std::string text(info.begin(),info.end());const auto content=std::filesystem::path(argv[2]).parent_path();auto mounts=gameinfoMountsCandidate(text,content,content.parent_path());
    RetainedSessionSceneCandidate scene(std::move(graph),root,mounts,4,true,true);
    std::vector<SceneModelAmbientRecordCandidate>records;size_t selectedModels=0,callbacks=0;int32_t tick=0;
    std::cout<<std::setprecision(9);
    scene.addSceneDecorationCandidate([&](const Graph&g,const SessionRenderSceneCandidate&s,const auto&geometry,const auto&,PreparedSessionSceneCandidate&prepared){
        const auto before=prepared.upload.vertices;const auto indices=prepared.upload.indices;
        const auto binding=readFilmSceneRenderBindingCandidate(g,s.shot);GraphTransformHierarchy hierarchy(g,binding.scene);
        std::vector<std::optional<std::array<float,3>>>points(geometry.size());std::vector<std::optional<uint32_t>>dags(geometry.size());
        size_t selected=0;
        for(size_t model=0;model<geometry.size();++model){
            const auto selection=sessionModelIlluminationDagCandidate(g,geometry[model].model.id,[&](uint32_t id){return g.strings.at(g.elements.at(id).type)=="DmeDag";});
            if(!selection.attributePresent)continue; // no guessed missing-attribute fallback
            require(selection.dag.has_value(),"benchmark explicit illumination DAG unresolved");
            require(hierarchy.ordinaryParents.count(*selection.dag)||*selection.dag==binding.scene,"illumination DAG outside scene scope");
            const auto matrix=hierarchy.world(*selection.dag);
            points[model]=std::array<float,3>{matrix[0][3],matrix[1][3],matrix[2][3]};dags[model]=selection.dag;++selected;
        }
        require(selected==1,"benchmark explicit lighting-DAG coverage changed");selectedModels+=selected;
        for(const auto*ambient:{&hdr,&ldr})for(int32_t center:{0,1}){
            std::vector<std::optional<GenericAmbientCubeCandidate>>cubes(geometry.size());
            for(size_t model=0;model<points.size();++model)if(points[model]){
                const auto result=sourceNewRecordLeafAmbientCandidate(*points[model],center,{4,false,0,0},cache,*ambient);cubes[model]=result.cube;
                std::cout<<"binding tick="<<tick<<" model="<<model<<" id="<<geometry[model].model.id<<" dag="<<*dags[model]<<" encoding="<<(ambient==&hdr?"hdr":"ldr")<<" cache_center="<<center<<" input_leaf="<<result.inputLeaf<<" query="<<(*points[model])[0]<<','<<(*points[model])[1]<<','<<(*points[model])[2]<<" sample="<<result.samplePoint[0]<<','<<result.samplePoint[1]<<','<<result.samplePoint[2]<<" cube=";
                for(const auto&face:result.cube)for(float channel:face)std::cout<<channel<<',';std::cout<<'\n';
            }
            const auto packed=prepareSceneModelAmbientRecordsCandidate(geometry,cubes);
            require(!packed.records.empty(),"bound model produced no ambient records");
            for(const auto&address:packed.addresses)require(points.at(address.model).has_value(),"unbound model received a guessed cube");
            records.insert(records.end(),packed.records.begin(),packed.records.end());
        }
        require(indices==prepared.upload.indices&&before.size()==prepared.upload.vertices.size()&&std::memcmp(before.data(),prepared.upload.vertices.data(),before.size()*sizeof(before[0]))==0,"ambient sampling changed model/world geometry");
        ++callbacks;
    });
    for(int32_t frameTick:{50000,50417}){tick=frameTick;auto frame=scene.sample(frameTick,1280,720);require(frame.upload.vertices.size()==52497&&frame.upload.indices.size()==110466,"benchmark geometry changed");}
    require(callbacks==2&&selectedModels==2&&!records.empty(),"bound model/frame coverage");
    std::ofstream output(argv[4],std::ios::binary);output.write(reinterpret_cast<const char*>(records.data()),std::streamsize(records.size()*sizeof(records[0])));require(bool(output),"record output failed");
    std::cout<<"PASS benchmark_frames=2 explicit_models_per_frame=1 cache_center_settings=2 bsp_hdr_cubes=4 bsp_ldr_cubes=4 records="<<records.size()<<" unbound_model_fallbacks=0 geometry_unchanged=1 native_runtime_selection=unverified\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
