#pragma once
// Test-only authored fixtures. No Valve models, textures or sessions are copied.
// Include after dmx_graph.cpp. No production/editor file includes this header.
#include "scene_material_inputs_candidate.h"
#include "scene_material_draw_inputs_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include <iostream>

inline void materialTestRequire(bool b,const char*message){if(!b)throw std::runtime_error(message);}
inline void materialTestGameInfo(){
 const std::filesystem::path game="C:/owned-game",mod=game/"usermod";
 const std::string source="GameInfo { FileSystem { SearchPaths { Game |gameinfo_path|. Game tf GameBin custom/bin } } }";
 auto plan=gameinfoMountPlanCandidate(source,mod,game);
 materialTestRequire(plan.gamePaths==std::vector<std::filesystem::path>{game/"usermod_english",mod,game/"tf_english",game/"tf"},"controlled GameInfo priority/language paths");
 materialTestRequire(plan.gameBinPaths==std::vector<std::filesystem::path>{mod/"bin",game/"tf/bin",game/"custom/bin"}&&plan.modPath==mod&&plan.defaultWritePath==mod,"controlled GameInfo derived paths");
 materialTestRequire(gameinfoMountsCandidate(source,mod,game,"")==std::vector<std::filesystem::path>{mod,game/"tf"},"controlled GameInfo no-language order");
 materialTestRequire(gameinfoMountsCandidate(source,mod,game,"french")==std::vector<std::filesystem::path>{game/"usermod_french",mod,game/"tf_french",game/"tf"},"controlled GameInfo explicit language");
 bool rejected=false;try{gameinfoMountsCandidate(source,mod,game,"../escape");}catch(const std::runtime_error&){rejected=true;}materialTestRequire(rejected,"GameInfo unsafe language refusal");
 std::cout<<"PASS controlled_gameinfo_checks=5\n";
}
inline std::vector<std::filesystem::path>materialTestInstalledMounts(const std::filesystem::path&content){
 std::ifstream file(content/"gameinfo.txt",std::ios::binary);if(!file)throw std::runtime_error("Integration fixture unavailable: content directory must contain readable gameinfo.txt");
 std::string text((std::istreambuf_iterator<char>(file)),{});auto mounts=gameinfoMountsCandidate(text,content,content.parent_path());
 materialTestRequire(!mounts.empty(),"installed GameInfo yielded no game paths");std::cout<<"integration_game_mounts="<<mounts.size()<<"\n";return mounts;
}
inline void materialTestWrite(const std::filesystem::path&path,const std::vector<uint8_t>&data){
 materialTestRequire(!std::filesystem::exists(path),"fresh owned fixture file required");std::filesystem::create_directories(path.parent_path());std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char*>(data.data()),data.size());materialTestRequire(bool(f),"owned fixture write");
}
inline std::filesystem::path materialTestMount(const char*directory){
 auto root=std::filesystem::weakly_canonical(std::filesystem::absolute(directory));auto outputs=std::filesystem::weakly_canonical(std::filesystem::current_path()/"outputs");auto relative=root.lexically_relative(outputs);
 materialTestRequire(!relative.empty()&&!relative.is_absolute(),"fixture directory must be inside workspace outputs");for(const auto&part:relative)materialTestRequire(part!="..","fixture directory outside workspace outputs");
 materialTestRequire(std::filesystem::is_directory(root)&&std::filesystem::is_empty(root),"fresh empty workspace fixture directory required");
 auto mount=root/"mount";
 auto texture=[&](const char*name,std::array<uint8_t,4>rgba){std::vector<uint8_t>b(84);auto put=[&](size_t at,uint32_t v){std::memcpy(b.data()+at,&v,4);};auto half=[&](size_t at,uint16_t v){std::memcpy(b.data()+at,&v,2);};put(0,0x00465456);put(4,7);put(8,2);put(12,80);half(16,1);half(18,1);half(24,1);half(63,1);put(52,0);b[56]=1;std::copy(rgba.begin(),rgba.end(),b.begin()+80);materialTestWrite(mount/"materials"/(std::string("fixture/")+name+".vtf"),b);};
 texture("base",{64,128,192,255});texture("alternate",{5,10,15,255});texture("glow",{128,64,32,255});
 const std::string vmt="VertexLitGeneric { \"$basetexture\" \"fixture/base\" }";
 materialTestWrite(mount/"materials/models/fixture/visor.vmt",{vmt.begin(),vmt.end()});
 materialTestWrite(mount/"materials/fixture/broken.vtf",{1,2,3});return mount;
}
inline SfmModelGeometryCandidate materialTestModel(){
 SfmModelGeometryCandidate m;m.geometry.vertices.resize(3);m.materials.names={"models/fixture/visor"};m.materials.directories={""};m.materials.skins={{0}};m.bodyModelCounts={1};m.bodyBases={1};SfmModelMeshGeometryCandidate mesh;mesh.triangles={{{0,1,2}}};m.meshes.push_back(mesh);return m;
}
inline Graph materialTestGraph(){
 Graph g;g.strings={"DmeGameModel","model","materials","DmeMaterial","material","mtlName","$basetexture","$alpha"};
 Value ref;ref.reference=1;Value name;name.text="models/fixture/visor";Value texture;texture.text="fixture/base";Value alpha;alpha.wire.resize(4);
 g.elements={{0,1,{},{{2,15,{ref}}}},{3,4,{},{{5,5,{name}},{6,5,{texture}},{7,3,{alpha}}}}};setFloat(g.elements[1].attributes[2],0,.7f);return g;
}
inline void materialTestSetTexture(Graph&g,const std::string&name){g.elements[1].attributes[1].values[0].text=name;}
inline void materialTestFallback(const ModelRenderMaterialCandidate&m,bool mask){
 if(mask){materialTestRequire(m.source.selfIllumMaskMissing&&m.source.selfIllumMaskTexture.empty()&&m.selfIllum.enabled&&m.selfIllum.mask.empty()&&m.selfIllumMaskImage.rgba.empty(),"missing mask diagnostic/base-alpha fallback");}
 else{const std::vector<uint8_t>expected{255,0,255,255,0,0,0,255,0,0,0,255,255,0,255,255};materialTestRequire(m.source.baseTextureMissing&&m.source.baseTexture.empty()&&m.image.width==2&&m.image.height==2&&m.image.rgba==expected,"missing base diagnostic/checker fallback");}
 materialTestRequire(!m.source.missingTextureReferences.empty(),"fallback must retain missing reference diagnostic");
}
