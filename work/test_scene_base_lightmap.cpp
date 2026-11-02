#define SFM64_SCENE_LIGHTMAP_NO_MAIN
#include "probe_d3d9_scene_base_lightmap_x64.cpp"
int main(int argc,char** argv){try{
    need(argc==2,"owned fixture directory only");RenderCameraCandidate camera{};camera.rows={1,0,0,0,0,1,0,0,0,0,0,.5f,0,0,0,1};
    auto scene=sceneLightLoad(argv[1],camera,0,0);const auto vertices=scene.upload.vertices;const auto indices=scene.upload.indices;
    const SceneBaseLightmapOptionsCandidate options({1,1,1});auto plan=prepareSceneBaseLightmapPlanCandidate(scene,options);
    need(plan.planes.size()==3&&plan.opaque.size()==4&&plan.luxels.size()==vertices.size(),"CPU actual scene lightmap plan");
    need(plan.opaque[0].plane==SIZE_MAX&&plan.opaque[1].draw.firstIndex==3&&plan.opaque[2].draw.firstIndex==6&&plan.opaque[3].draw.firstIndex==9,"CPU ordered split/nonzero ranges");
    need(plan.planes[0]->faceIndex()==0&&plan.planes[1]->faceIndex()==2&&plan.planes[2]->faceIndex()==1,"CPU regrouped source identities");
    need(scene.upload.indices==indices&&std::memcmp(scene.upload.vertices.data(),vertices.data(),vertices.size()*sizeof(vertices[0]))==0,"CPU planner changed input geometry");
    size_t negative=0;auto refuses=[&](auto call){bool failed=false;try{call();}catch(const std::exception&){failed=true;}need(failed,"CPU invalid scene lightmap accepted");++negative;};
    for(int bad=0;bad<6;++bad){auto edited=scene;if(bad==0)edited.upload.bspLightmaps.clear();if(bad==1)edited.upload.vertexShading[3].origin=SceneShadingOriginCandidate::GeneratedGeometry;if(bad==2)edited.upload.vertexShading[3].sourceVertex=99;if(bad==3)edited.upload.bspLightmaps[0].firstIndex=0;if(bad==4)edited.materialAlpha[1]=.5f;if(bad==5)edited.upload.indices[3]=0;refuses([&]{prepareSceneBaseLightmapPlanCandidate(edited,options);});}
    refuses([&]{prepareSceneBaseLightmapPlanCandidate(scene,SceneBaseLightmapOptionsCandidate({-1,1,1}));});
    std::weak_ptr<const SourceBspLightmapSnapshotCandidate> owner=scene.upload.bspLightmaps[0].snapshot;scene={};need(!owner.expired()&&plan.planes[0]->pixels().size()==4,"CPU plan retained source after scene/cache");plan={};need(owner.expired(),"CPU plan source release");
    std::cout<<"PASS scene_lightmap_cpu=1 actual_retained_world=1 split_batches=4 unique_planes=3 refusal_checks="<<negative<<" geometry_unchanged=1 lifetime=1 native_x64=1\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
