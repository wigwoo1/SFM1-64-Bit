#define NOMINMAX
#include <windows.h>
#define SFM64_RETAINED_HDR2_PROFILE_NO_MAIN
#include "test_retained_hdr2_profile.cpp"
#include "editor_hdr2_profile_candidate.h"
#include <sstream>

static std::string profileText(uint32_t encoding=113,const char*filter="nearest",float exposure=1){
 std::ostringstream s;s<<"SFM64_HDR2_PROFILE_V1\nselectedHdrType 2\nencoding "<<encoding<<"\nstyleScale 0.5\ninstance 0.5 0.75 1 1\nconfigurationByte49 0\nexposure "<<exposure<<"\npagePadding 0\npageInitialByte 0\nfilter "<<filter<<'\n';return s.str();
}
#ifndef SFM64_EDITOR_HDR2_PROFILE_ENTRY
#define SFM64_EDITOR_HDR2_PROFILE_ENTRY main
#endif
int SFM64_EDITOR_HDR2_PROFILE_ENTRY(int argc,char**argv){try{
 check(argc==3,"fresh owned directory and VTF");const std::filesystem::path root=argv[1];check(std::filesystem::create_directory(root),"fresh fixture directory");
 std::istringstream valid(profileText());auto p=readEditorHdr2ProfileCandidate(valid);check(p->policy().encoding==113&&p->policy().instance==std::array<float,4>{.5f,.75f,1,1}&&p->filter()==SceneHdr2FilterCandidate::Nearest,"explicit parsed policy");
 std::istringstream numeric(profileText(36,"linear",2));auto q=readEditorHdr2ProfileCandidate(numeric);check(q->policy().encoding==36&&q->filter()==SceneHdr2FilterCandidate::Linear&&q->policy().exposure==2,"numeric linear policy");
 size_t refused=0;for(const auto& change:std::vector<std::pair<std::string,std::string>>{{"PROFILE_V1","PROFILE_V2"},{"selectedHdrType 2","selectedHdrType 1"},{"encoding 113","encoding 99"},{"styleScale 0.5","styleScale -1"},{"instance 0.5 0.75 1 1","instance 0.5 0.75 1 0.5"},{"configurationByte49 0","configurationByte49 256"},{"exposure 1","exposure 0"},{"pagePadding 0","pagePadding 16384"},{"pageInitialByte 0","pageInitialByte 256"},{"filter nearest","filter cubic"},{"exposure 1","exposure nan"},{"encoding 113","encoding 99999999999999999999999"},{"instance 0.5 0.75 1 1","instance .5"}}){auto text=profileText();auto at=text.find(change.first);check(at!=std::string::npos,"bad test token");text.replace(at,change.first.size(),change.second);std::istringstream input(text);refuses([&]{readEditorHdr2ProfileCandidate(input);},"invalid explicit profile accepted");++refused;}
 for(const auto& text:{std::string{},profileText()+"extra",profileText().substr(0,profileText().find("filter"))}){std::istringstream input(text);refuses([&]{readEditorHdr2ProfileCandidate(input);},"incomplete/extra profile accepted");++refused;}
 std::filesystem::create_directories(root/"maps");std::filesystem::create_directories(root/"materials/owned");putFile(root/"materials/owned/pixel.vtf",readBinaryAssetCandidate(argv[2]));
 for(const char*n:{"mat","other"}){std::ofstream f(root/"materials"/(std::string(n)+".vmt"));f<<"LightmappedGeneric { $basetexture owned/pixel }";}
 {std::ofstream f(root/"gameinfo.txt");f<<"GameInfo { FileSystem { SearchPaths { Game |gameinfo_path|. } } }";}
 auto bytes=threeFaces();const auto ldrAt=get32(bytes,8+7*16);std::vector<uint8_t> ldr(4*56);for(size_t i=1;i<4;++i)std::copy_n(bytes.data()+ldrAt+56,56,ldr.data()+i*56);replace(bytes,7,ldr,1);putFile(root/"maps/owned.bsp",bytes);
 auto unavailable=bytes;const auto faces=get32(unavailable,8+58*16);for(size_t i=1;i<4;++i){unavailable[faces+i*56+16]=255;u32(unavailable,faces+i*56+20,uint32_t(-1));}putFile(root/"maps/unavailable.bsp",unavailable);
 for(const char*map:{"owned","unavailable"}){
  auto g=profileGraph(map);g.header="<!-- dmx encoding binary 5 format sfm_session 20 -->";
  for(auto& a:g.elements[3].attributes){if(g.strings[a.name]=="position"){const float v[]{-10,0,10};std::memcpy(a.values[0].wire.data(),v,12);}if(g.strings[a.name]=="orientation"){const float v[]{0,.38268343f,0,.9238795f};std::memcpy(a.values[0].wire.data(),v,16);}}
  const uint32_t order[]{5,0,1,2,3,4,6,7};uint32_t remap[8]{};for(uint32_t n=0;n<8;++n)remap[order[n]]=n;auto original=g.elements;for(size_t n=0;n<8;++n)g.elements[n]=original[order[n]];
  for(auto&e:g.elements)for(auto&a:e.attributes)for(auto&v:a.values){if((a.type==1||a.type==15)&&v.reference<8)v.reference=remap[v.reference];if(a.type==5)v.stringIndex=internString(g,v.text);}
  auto path=(root/(std::string(map)+".dmx")).string();saveGraph(g,path.c_str());check(graphExactEqualCandidate(g,loadGraph(path.c_str())),"owned fixture round trip");
 }
 for(uint32_t encoding:{113u,36u})for(const char*filter:{"nearest","linear"})for(float exposure:{1.f,2.f}){std::ofstream f(root/(std::to_string(encoding)+"-"+filter+"-"+std::to_string(int(exposure))+".profile"));f<<profileText(encoding,filter,exposure);}
 RetainedSessionSceneCandidate owner(loadGraph((root/"owned.dmx").string().c_str()),1,{root},1,false,false,p);
 auto frame=owner.sample(0,64,32);check(editorHdr2SummaryCandidate(*frame.hdr2)=="Experimental HDR2: 3 lit faces, 0 fallback batches. Source appearance unverified.","active route status");
 owner.replaceSession(loadGraph((root/"unavailable.dmx").string().c_str()),1);auto fallback=owner.sample(0,64,32);check(fallback.hdr2->plan()->faces.empty()&&editorHdr2SummaryCandidate(*fallback.hdr2).find("0 lit faces")!=std::string::npos,"fallback route status");
 editorHdr2ProfileCandidate=p;SetEnvironmentVariableA("SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK","0");SetEnvironmentVariableA("SFM64_EXPERIMENTAL_HDR2_PROFILE",(root/"absent.profile").string().c_str());refuses([&]{configureEditorHdr2ProfileCandidate();},"missing startup profile accepted");check(editorHdr2ProfileCandidate==p,"failed startup replaced committed profile");SetEnvironmentVariableA("SFM64_EXPERIMENTAL_HDR2_PROFILE",(root/"36-linear-2.profile").string().c_str());configureEditorHdr2ProfileCandidate();check(editorHdr2ProfileCandidate->identity()==q->identity(),"startup did not publish complete explicit profile");SetEnvironmentVariableA("SFM64_EXPERIMENTAL_HDR2_PROFILE",nullptr);SetEnvironmentVariableA("SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK",nullptr);editorHdr2ProfileCandidate.reset();
 std::cout<<"PASS editor_hdr2_explicit_profile=1 invalid_profiles="<<refused<<" fixture_roundtrip=2 source_write_scope=owned_only\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
