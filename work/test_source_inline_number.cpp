#define SFM64_INLINE_MAP_CACHE_NO_MAIN
#include "test_inline_map_cache.cpp"

int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"native x64 required");
 if(argc==3&&std::string(argv[1])=="--audit-session"){
  const auto g=loadGraph(argv[2]);size_t missing=0;
  for(const auto&e:g.elements)if(g.strings.at(e.type)=="DmeGameModel"){
   auto a=namedAttribute(g,e,"modelName");if(a&&a->type==5&&a->values.size()==1&&a->values[0].text=="*17"){
    check(!namedAttribute(g,e,"effects"),"benchmark missing-inline effects path changed");++missing;
   }
  }
  check(missing==13,"benchmark missing-inline attribute coverage");
  std::cout<<"PASS benchmark_unavailable_inline_nodes=13 effects_absent=13 original_runtime=ABSENT\n";return 0;
 }check(argc==3||argc==4,"fresh owned directory, VTF and optional --expect-previous");
 const bool previous=argc==4;check(!previous||std::string(argv[3])=="--expect-previous","unknown test option");
 const std::filesystem::path root=argv[1];check(std::filesystem::create_directory(root),"fresh owned directory");std::filesystem::create_directories(root/"maps");std::filesystem::create_directories(root/"materials/owned");putFile(root/"materials/owned/pixel.vtf",readBinaryAssetCandidate(argv[2]));
 for(const char*n:{"mat","other"}){std::ofstream f(root/"materials"/(std::string(n)+".vmt"));f<<"LightmappedGeneric { $basetexture owned/pixel }";}
 auto bytes=threeFaces();const auto ldrAt=get32(bytes,8+7*16);std::vector<uint8_t> ldr(4*56);for(size_t i=1;i<4;++i)std::copy_n(bytes.data()+ldrAt+56,56,ldr.data()+i*56);replace(bytes,7,ldr,1);
 // Three deliberately different literal record spans: world0 has three faces,
 // inline1 one face, inline2 two faces. This detects an off-by-one selection.
 const auto model=get32(bytes,8+14*16);std::vector<uint8_t> models(3*48);for(size_t n=0;n<3;++n)std::copy_n(bytes.data()+model,48,models.data()+48*n);
 u32(models,48+40,1);u32(models,48+44,1);u32(models,96+40,2);u32(models,96+44,2);replace(bytes,14,models);putFile(root/"maps/owned.bsp",bytes);
 auto graph=[](const char* token){auto g=inlineGraph("owned");for(auto&a:g.elements[8].attributes)if(g.strings.at(a.name)=="modelName")a.values[0].text=token;return g;};
 size_t checked=0,refused=0;
 for(bool hdr:{false,true}){
  std::shared_ptr<const SceneHdr2ProfileCandidate> profile;
  if(hdr)profile=std::make_shared<const SceneHdr2ProfileCandidate>(SourceHdr2ScenePolicyCandidate{2,113,.5f,{1,1,1,1},0,1},0u,uint8_t(0),SceneHdr2FilterCandidate::Nearest);
  const auto g1=graph("*1"),g2=graph("*2");RetainedSessionSceneCandidate owner(g1,0,{root},1,false,false,profile);
  auto first=owner.sample(0,64,32);owner.replaceSession(g2,0);auto second=owner.sample(0,64,32);
  const size_t expected1=previous?(hdr?11u:9u):(hdr?4u:3u),expected2=previous?(hdr?4u:3u):(hdr?7u:6u);
  check(modelVertices(first,0)==expected1&&modelVertices(second,0)==expected2,"inline number selected wrong literal model record");
  check(modelVertices(first,1)==(hdr?11u:9u)&&modelVertices(second,1)==(hdr?11u:9u),"world record0 selection changed");
  const auto* retained=&first.materialInputs.at(0);owner.replaceSession(g1,0);auto restored=owner.samplePlayback(416,64,32);sameFrame(first,restored);check(&restored.materialInputs.at(0)==retained,"numbered model cache restore changed identity");
  if(previous){owner.replaceSession(graph("*3"),0);auto offEnd=owner.sample(0,64,32);check(modelVertices(offEnd,0)==(hdr?7u:6u),"old inclusive off-by-one upper bound not reproduced");std::cout<<"REPRODUCED previous_inline_number_minus_one=1 profile="<<(hdr?"hdr":"legacy")<<" star1_world=1 upper_bound_wrong=1\n";}
  else {
   owner.replaceSession(graph("*3"),0);auto absent=owner.sample(0,64,32);
   check(absent.missingModels.size()==1&&absent.missingModels[0].id==8&&absent.missingModels[0].asset=="*3"&&absent.missingModels[0].requestedIndex==3&&absent.missingModels[0].modelCount==3,"missing-inline diagnostic identity");
   check(absent.materialInputs.size()==1&&absent.upload.bspLightmaps[0].model==0&&modelVertices(absent,0)==(hdr?11u:9u),"missing inline drew substitute geometry");
   check(sceneMissingAssetSummaryCandidate(absent.missingAssets,absent.missingModels).find("*3")!=std::string::npos,"missing inline not reported in UI summary");
   const std::vector<SceneMissingAssetDiagnosticCandidate> textures{{"test",{}, {"missing.vtf"}}};auto summary=sceneMissingAssetSummaryCandidate(textures,absent.missingModels);
   check(summary.find("*3")!=std::string::npos&&summary.find("missing.vtf")!=std::string::npos,"model diagnostic hid texture diagnostic");
   auto effects=graph("*3");effects.strings.push_back("effects");effects.elements[8].attributes.push_back({uint32_t(effects.strings.size()-1),15,{}});owner.replaceSession(effects,0);
   refuses([&]{owner.sample(0,64,32);},"unverified effects fallback silently admitted");++refused;
   for(const char* token:{"*0","*999999999999999999999999","*-1","*+1","*1tail","*1 ","*"}){owner.replaceSession(graph(token),0);refuses([&]{owner.sample(0,64,32);},"malformed inline number accepted");++refused;}
   owner.replaceSession(g1,0);auto recovered=owner.sample(0,64,32);sameFrame(first,recovered);check(&recovered.materialInputs.at(0)==retained,"missing/error admission invalidated old cache");
  }
  if(hdr&&!previous)check(first.hdr2&&second.hdr2&&first.hdr2->plan()->faces.size()==3&&second.hdr2->plan()->faces.size()==3,"HDR world plan changed by corrected inline number");
  ++checked;
 }
 check(readBinaryAssetCandidate(root/"maps/owned.bsp")==bytes,"owned source changed");
 std::cout<<(previous?"PASS previous_inline_number=1":"PASS source_inline_number=1")<<" profiles="<<checked<<" malformed_or_unsupported_refusals="<<refused<<" unavailable_inline_diagnostics="<<(previous?0:2)<<" world0=preserved original_runtime=ABSENT\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
