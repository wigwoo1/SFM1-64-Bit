#define SFM64_RETAINED_HDR2_PROFILE_NO_MAIN
#include "test_retained_hdr2_profile.cpp"

static Graph inlineGraph(const char* map){
 auto g=profileGraph(map);
 auto name=[&](const char* text){auto it=std::find(g.strings.begin(),g.strings.end(),text);if(it!=g.strings.end())return uint32_t(it-g.strings.begin());g.strings.emplace_back(text);return uint32_t(g.strings.size()-1);};
 for(const char*type:{"DmeGameModel","DmeTransform"}){Element e{};e.type=name(type);e.name=e.type;e.id[0]=char(g.elements.size()+1);g.elements.push_back(e);}
 auto ref=[](uint32_t id){Value v;v.reference=id;return v;};
 auto attr=[&](uint32_t id,const char* key,uint8_t type,std::vector<Value> values){g.elements.at(id).attributes.push_back({name(key),type,std::move(values)});};
 auto words=[](std::initializer_list<float> values){Value v;v.wire.resize(values.size()*4);std::memcpy(v.wire.data(),values.begin(),v.wire.size());return v;};
 Value zero;zero.wire.resize(4);Value visible;visible.wire={1};Value model;model.text="*1";
 attr(1,"children",15,{ref(8)});attr(8,"modelName",5,{model});attr(8,"transform",1,{ref(9)});attr(8,"skin",2,{zero});attr(8,"body",2,{zero});attr(8,"visible",4,{visible});attr(8,"bones",15,{});
 attr(9,"position",10,{words({0,0,0})});attr(9,"orientation",13,{words({0,0,0,1})});return g;
}
static size_t modelVertices(const PreparedSessionSceneCandidate& f,size_t model){
 return size_t(std::count_if(f.upload.vertexShading.begin(),f.upload.vertexShading.end(),[&](const auto&v){return v.model==model;}));
}
static void mapFixture(const std::filesystem::path&root,const std::vector<uint8_t>& a){
 putFile(root/"maps/a.bsp",a);auto b=a;
 // Same inline name, but independent literal geometry/material identities.
 const auto model=get32(b,8+14*16);u32(b,model+44,1);u32(b,model+48+44,1);
 const auto vertices=get32(b,8+3*16);for(size_t i=0;i<5;++i)f32(b,vertices+i*12,20.f+float(i));
 std::vector<uint8_t> names{'a','l','t','e','r','n','a','t','e',0,'s','e','c','o','n','d',0},table(8);u32(table,4,10);replace(b,43,names);replace(b,44,table);
 putFile(root/"maps/b.bsp",b);
 auto invalid=b;u32(invalid,12+58*16,0);u32(invalid,12+7*16,0);putFile(root/"maps/invalid.bsp",invalid);
}
#ifndef SFM64_INLINE_MAP_CACHE_NO_MAIN
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"native x64 required");check(argc==3||argc==4,"fresh root, VTF and optional --expect-stale");bool stale=argc==4;check(!stale||std::string(argv[3])=="--expect-stale","unknown test option");
 const std::filesystem::path root=argv[1];check(std::filesystem::create_directory(root),"fresh fixture directory");std::filesystem::create_directories(root/"maps");std::filesystem::create_directories(root/"materials/owned");putFile(root/"materials/owned/pixel.vtf",readBinaryAssetCandidate(argv[2]));
 for(const char* n:{"mat","other","alternate","second"}){std::ofstream f(root/"materials"/(std::string(n)+".vmt"));f<<"LightmappedGeneric { $basetexture owned/pixel }";}
 auto bytes=threeFaces();const auto ldrAt=get32(bytes,8+7*16);std::vector<uint8_t> ldr(4*56);for(size_t i=1;i<4;++i)std::copy_n(bytes.data()+ldrAt+56,56,ldr.data()+i*56);replace(bytes,7,ldr,1);
 // Inline '*1' names record1; keep this cache fixture's original per-map
 // shapes/material assertions while declaring that record explicitly.
 const auto record=get32(bytes,8+14*16);std::vector<uint8_t> models(96);
 std::copy_n(bytes.data()+record,48,models.data());std::copy_n(bytes.data()+record,48,models.data()+48);
 replace(bytes,14,models);mapFixture(root,bytes);
 const auto sourceA=readBinaryAssetCandidate(root/"maps/a.bsp"),sourceB=readBinaryAssetCandidate(root/"maps/b.bsp");
 size_t modes=0;
 for(bool hdr:{false,true}){
  std::shared_ptr<const SceneHdr2ProfileCandidate> profile;
  if(hdr)profile=std::make_shared<const SceneHdr2ProfileCandidate>(SourceHdr2ScenePolicyCandidate{2,113,.5f,{1,1,1,1},0,1},0u,uint8_t(0),SceneHdr2FilterCandidate::Nearest);
  auto ga=inlineGraph("a"),gb=inlineGraph("b");const auto original=ga;
  RetainedSessionSceneCandidate owner(ga,0,{root},1,false,false,profile);
  auto a=owner.samplePlayback(0,64,32);check(modelVertices(a,0)==(hdr?11u:9u)&&a.materialInputs.at(0).materials.size()==(hdr?2u:1u),"A inline fixture coverage");
  const auto* aInput=&a.materialInputs.at(0);const auto aIndices=aInput->indices;const auto aPath=aInput->materials.at(0).source.vmt;const auto aVertices=a.upload.vertices;
  owner.replaceSession(gb,0);auto b=owner.samplePlayback(0,64,32);
  const bool staleGeometry=modelVertices(b,0)==modelVertices(a,0);
  const bool staleMaterials=&b.materialInputs.at(0)==aInput;
  if(stale){check(staleGeometry&&staleMaterials,"pre-correction stale cache not reproduced");std::cout<<"REPRODUCED inline_map_cache_stale=1 profile="<<(hdr?"hdr":"legacy")<<" geometry=1 material_pointer=1\n";++modes;continue;}
  check(modelVertices(b,0)==(hdr?4u:3u)&&!staleGeometry&&!staleMaterials,"B reused A inline geometry/materials");
  check(b.materialInputs.at(0).materials.at(0).source.vmt.stem()==(hdr?"alternate":"second"),"B material source identity");
  check(b.upload.vertices.at(0).position[0]>=20&&a.upload.vertices.at(0).position[0]<20,"B source positions not selected");
  check(aInput->indices==aIndices&&aInput->materials.at(0).source.vmt==aPath&&a.upload.vertices.size()==aVertices.size(),"old borrowed material/frame invalidated");
  for(size_t n=0;n<aVertices.size();++n)check(a.upload.vertices[n].position==aVertices[n].position,"old geometry frame changed");
  check(owner.assetCount()==2&&owner.materialCount()==2,"per-map cache entries not retained");
  // Repeated seeks/map returns must reuse A's original material entry. Old A
  // and B packets remain alive while the retained scene switches maps.
  owner.replaceSession(ga,0);auto restored=std::async(std::launch::async,[&]{return owner.samplePlayback(416,64,32);}).get();
  check(&restored.materialInputs.at(0)==aInput&&owner.assetCount()==2&&owner.materialCount()==2,"A return did not reuse stable cache");sameFrame(a,restored);
  owner.replaceSession(inlineGraph("invalid"),0);refuses([&]{owner.sample(0,64,32);},"invalid selected dataset accepted");
  check(aInput->indices==aIndices&&aInput->materials.at(0).source.vmt==aPath&&owner.assetCount()==2&&owner.materialCount()==2,"failed map load changed retained cache");
  owner.replaceSession(inlineGraph("../outside"),0);refuses([&]{owner.sample(0,64,32);},"unsafe map traversal accepted");
  owner.replaceSession(ga,0);sameFrame(a,owner.sample(0,64,32));
  // Equivalent spelling resolves to the same existing map, without a duplicate
  // asset/material entry or invalidation of either previously borrowed packet.
  owner.replaceSession(inlineGraph("./a.bsp"),0);auto alias=owner.sample(0,64,32);sameFrame(a,alias);check(&alias.materialInputs.at(0)==aInput&&owner.assetCount()==2&&owner.materialCount()==2,"same resolved map alias changed cache identity");
  if(hdr){check(a.hdr2&&b.hdr2&&restored.hdr2&&a.hdr2->profile()==profile&&b.hdr2->profile()==profile,"HDR profile lost through map switch");check(a.upload.bspLightmaps[0].snapshot->lightmaps->faceLump()==58&&b.upload.bspLightmaps[0].snapshot->lightmaps->faceLump()==58,"profile world dataset drift");}
  check(graphExactEqualCandidate(ga,original),"input graph mutated");++modes;
 }
 check(readBinaryAssetCandidate(root/"maps/a.bsp")==sourceA&&readBinaryAssetCandidate(root/"maps/b.bsp")==sourceB,"fixture source modified");
 std::cout<<(stale?"PASS stale_reproduction=1":"PASS inline_map_cache=1")<<" profiles="<<modes<<" original_runtime=ABSENT index_convention=direct_BSP_record\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

#endif
