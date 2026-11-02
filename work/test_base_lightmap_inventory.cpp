#define SFM64_INVENTORY_NO_MAIN
#include "probe_base_lightmap_inventory_x64.cpp"
int main(int argc,char** argv){try{
    inventoryNeed(argc==2,"owned fixture directory");const std::filesystem::path root=argv[1];const auto bytes=readBinaryAssetCandidate(root/"maps/owned.bsp");const InventoryRaw raw(bytes);auto scene=inventoryLoad({root},"owned");auto result=inventoryWorld(scene,raw,{root},bytes);
    inventoryNeed(result.faces.size()==3&&result.materials.size()==2&&result.ranges.size()==2&&result.indices==9,"owned baseline dimensions");for(const auto& f:result.faces)inventoryNeed(f.combined.empty()&&f.triangles==1,"owned baseline admitted");
    const auto material=scene.materialInputs.at(0).materials[0];size_t cases=0,defects=0;
    for(int kind=0;kind<5;++kind){auto m=material;InventoryReasons expected;
        if(kind==0){m.source.syntax.key="VertexLitGeneric";expected={"material.shader"};}
        if(kind==1){m.source.syntax.children.push_back({"$bumpmap","owned/b",{},false});m.source.syntax.children.push_back({"$phong","1",{},false});expected={"material.shape","material.parameter:$bumpmap","material.parameter:$phong"};}
        if(kind==2){m.preparedBase.cpu.reset();expected={"texture.no_prepared_plan"};}
        if(kind==3){m.source.baseTextureMissing=true;m.preparedBase.cpu.reset();expected={"material.missing_base","texture.no_prepared_plan"};}
        if(kind==4){m.translucent=true;m.alphaCutoff=.5f;expected={"material.translucent","material.alpha_test"};}
        inventoryNeed(inventoryMaterialReasons(m,1)==expected,"known material reasons including overlap");++cases;
    }
    auto fatal=[&](auto call){bool failed=false;try{call();}catch(const std::exception&){failed=true;}inventoryNeed(failed,"defect incorrectly accepted as unsupported");++defects;};
    for(int kind=0;kind<6;++kind){auto copy=scene;if(kind==0)copy.upload.bspLightmaps.clear();if(kind==1)copy.upload.indices[0]=UINT32_MAX;if(kind==2)copy.upload.vertexShading[0].sourceVertex=99;if(kind==3)copy.upload.vertexShading[0].origin=SceneShadingOriginCandidate::GeneratedGeometry;if(kind==4)copy.upload.opaque[0].materialSlot=UINT32_MAX;if(kind==5)copy.upload.opaque[0].firstIndex+=3;fatal([&]{inventoryWorld(copy,raw,{root},bytes);});}
    {auto m=material;m.preparedBase.frame=UINT32_MAX;fatal([&]{inventoryMaterialReasons(m,1);});}
    std::ostringstream json;inventoryWrite(result,json);inventoryNeed(json.str().find("world base-lightmap contract only")!=std::string::npos,"inventory scope label");
    std::cout<<"PASS inventory_cpu=1 known_material_cases="<<cases<<" defect_refusals="<<defects<<" owned_faces=3 owned_triangles=3 overlap=1 original_runtime=ABSENT native_x64=1\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
