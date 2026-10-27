#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_material_binding_candidate.h"
#include "scene_material_draw_inputs_candidate.h"
#include <iostream>
#include <iomanip>

static void requireIdentity(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static bool equalIdentityValues(const Value&a,const Value&b){return a.wire==b.wire&&a.text==b.text&&a.reference==b.reference&&a.stringIndex==b.stringIndex;}
static void verifyIdentityPayload(const Graph&g,const SessionMaterialOverrideCandidate&record){
 const auto&e=g.elements.at(record.element);size_t count=0;
 for(const auto&a:e.attributes){auto name=g.strings.at(a.name);if(name.empty()||name[0]!='$')continue;for(char&c:name)if(c>='A'&&c<='Z')c=char(c+32);
  const auto&p=record.parameters.at(name);requireIdentity(p.type==a.type&&p.values.size()==a.values.size(),"typed parameter shape changed");
  for(size_t i=0;i<a.values.size();++i)requireIdentity(equalIdentityValues(a.values[i],p.values[i]),"typed value bytes/identity changed");++count;
 }requireIdentity(count==record.parameters.size(),"parameter count changed");
}
static int auditIdentity(const char*session,const char*destination){
 requireIdentity(!std::ifstream(destination).good(),"fresh audit destination");auto g=loadGraph(session);std::ofstream out(destination,std::ios::binary);requireIdentity(bool(out),"audit output");
 unsigned names=0,errors=0,models=0,positions=0,modelErrors=0;
 for(uint32_t i=0;i<g.elements.size();++i){const auto&e=g.elements[i];if(g.strings.at(e.type)=="DmeMaterial"){
  auto*a=namedAttribute(g,e,"mtlName");requireIdentity(a&&a->type==5&&a->values.size()==1,"material name schema");++names;
  try{auto key=normalizeSessionMaterialNameCandidate(a->values[0].text);out<<"M\t"<<i<<'\t'<<a->values[0].text<<'\t'<<key<<'\n';}
  catch(const std::runtime_error&error){out<<"R\t"<<i<<'\t'<<error.what()<<'\n';++errors;}
 }if(g.strings.at(e.type)=="DmeGameModel"){
  auto*a=namedAttribute(g,e,"materials");if(!a)continue;requireIdentity(a->type==15,"model material schema");++models;
  for(size_t p=0;p<a->values.size();++p){out<<"O\t"<<i<<'\t'<<p<<'\t'<<a->values[p].reference<<'\n';++positions;}
  try{auto records=readSessionMaterialOverridesCandidate(g,i);requireIdentity(records.size()==a->values.size(),"model positions changed");
   for(size_t p=0;p<records.size();++p){requireIdentity(records[p].element==a->values[p].reference,"source order changed");verifyIdentityPayload(g,records[p]);out<<"P\t"<<i<<'\t'<<p<<'\t'<<records[p].element<<'\t'<<records[p].material<<'\t'<<records[p].parameters.size()<<'\n';}
  }catch(const std::runtime_error&error){out<<"E\t"<<i<<'\t'<<error.what()<<'\n';++modelErrors;}
 }}
 out.flush();requireIdentity(bool(out),"audit write complete");std::cout<<"PASS identity_audit names="<<names<<" rejected="<<errors<<" arrays="<<models<<" positions="<<positions<<" reader_errors="<<modelErrors<<'\n';return 0;
}
static Graph identityGraph(const std::vector<std::string>&names){
 Graph g;g.strings={"DmeGameModel","model","materials","DmeMaterial","material","mtlName","$Float","$Text"};
 Element model{0,1,{},{}};Attribute refs{2,15,{}};
 for(size_t i=0;i<names.size();++i){Value ref;ref.reference=uint32_t(i+1);refs.values.push_back(ref);Value name;name.text=names[i];name.stringIndex=123;
  Value number;number.wire.resize(4);uint32_t bits=0x7fc12345;std::memcpy(number.wire.data(),&bits,4);Value text;text.text="Keep Text";text.stringIndex=99;
  g.elements.push_back({3,4,{},{{5,5,{name}},{6,3,{number}},{7,5,{text}}}});
 }model.attributes.push_back(refs);g.elements.insert(g.elements.begin(),model);return g;
}
static int safetyIdentity(const char*directory){
 static_assert(sizeof(void*)==8,"owned x64 test");size_t checks=0;auto check=[&](bool b,const char*s){requireIdentity(b,s);++checks;};
 for(const auto&p:std::vector<std::pair<std::string,std::string>>{{"Models\\Example\\BODY.vmt","models/example/body"},{"/Models/A.VMT","/models/a"},{"A/B.PNG","a/b"},{"A/B.Foo.Bar","a/b.foo"},{".Hidden",".hidden"},{"Dir/.Hidden","dir/"},{"foo//BAR.vmt","foo//bar"},{"foo/../BAR.vmt","foo/../bar"},{"C:\\A.VMT","c:/a"},{"",""},{".","."},{"..","."},{std::string("A.VMT\0tail",10),"a"}})check(normalizeSessionMaterialNameCandidate(p.first)==p.second,"normalization example");
 bool rejected=false;try{normalizeSessionMaterialNameCandidate(std::string(4096,'x'));}catch(const std::runtime_error&){rejected=true;}check(rejected,"oversize identity refused");
 check(normalizeSessionMaterialNameCandidate(std::string(4095,'X'))==std::string(4095,'x'),"resource key independent of filename limit");
 auto g=identityGraph({"MODELS\\A.VMT","/models/a"});auto records=readSessionMaterialOverridesCandidate(g,0);check(records.size()==2&&records[0].material=="models/a"&&records[1].material=="/models/a","leading slash distinct and ordered");
 for(const auto&r:records)verifyIdentityPayload(g,r);++checks;
 auto key=sessionMaterialOverrideKeyCandidate(records);auto reverse=records;std::reverse(reverse.begin(),reverse.end());check(sessionMaterialOverrideKeyCandidate(reverse)==key,"cache key order policy unchanged");
 auto changed=records;changed[0].parameters.at("$text").values[0].text+="!";check(sessionMaterialOverrideKeyCandidate(changed)!=key,"exact payload cache invalidation");
 rejected=false;try{readSessionMaterialOverridesCandidate(identityGraph({"models/a","MODELS\\A.PNG"}),0);}catch(const std::runtime_error&e){rejected=std::string(e.what())=="duplicate model material override";}check(rejected,"duplicate normalized records still refused");
 rejected=false;try{sessionMaterialOverrideKeyCandidate({records[0],records[0]});}catch(const std::runtime_error&){rejected=true;}check(rejected,"duplicate cache key still refused");
 auto empty=readSessionMaterialOverridesCandidate(identityGraph({"",".Hidden","dir/.Hidden"}),0);check(empty[0].material.empty()&&empty[1].material==".hidden"&&empty[2].material=="dir/","empty/dot graph names preserved");
 // Everything created below stays inside a dedicated fresh workspace folder.
 std::filesystem::path root=std::filesystem::absolute(directory);requireIdentity(std::filesystem::is_directory(root)&&std::filesystem::is_empty(root),"fresh test directory required");
 auto mount=root/"mount";std::filesystem::create_directories(mount/"materials/models");auto vmt=mount/"materials/models/a.vmt";
 {std::ofstream f(vmt);f<<"VertexLitGeneric { \"$basetexture\" \"fixture/base\" }";requireIdentity(bool(f),"fixture VMT");}
 {std::ofstream f(root/"outside.vmt");f<<"DO NOT READ OR MODIFY";requireIdentity(bool(f),"fixture sentinel");}
 auto parameter=[](const char*s){Value v;v.text=s;return SessionMaterialParameterCandidate{5,{v}};};
 SessionMaterialOverrideCandidate ordinary{1,"models/a",{{"$basetexture",parameter("fixture/right")}}};
 SessionMaterialOverrideCandidate slash{2,"/models/a",{{"$basetexture",parameter("fixture/wrong")}}};
 auto syntax=parseVmtCandidate("VertexLitGeneric { \"$basetexture\" \"fixture/base\" }");auto binding=sessionMaterialBindingCandidate({ordinary,slash},{mount});binding(vmt,syntax);
 check(vmtValuesCandidate(syntax,"$basetexture")==std::vector<std::string>{"fixture/right"},"binding does not alias leading slash");
 syntax=parseVmtCandidate("VertexLitGeneric { \"$basetexture\" \"fixture/base\" }");auto slashOnly=sessionMaterialBindingCandidate({slash},{mount});slashOnly(vmt,syntax);
 check(vmtValuesCandidate(syntax,"$basetexture")==std::vector<std::string>{"fixture/base"},"unmatched leading slash does not affect ordinary material");
 rejected=false;try{binding(root/"outside.vmt",syntax);}catch(const std::runtime_error&){rejected=true;}check(rejected,"binding mount containment retained");
 rejected=false;try{binding(mount/"materials/../../outside.vmt",syntax);}catch(const std::runtime_error&){rejected=true;}check(rejected,"binding lexical traversal refused");
 for(const auto&name:{"../outside","models/../../outside","C:/outside","/outside","\\\\server\\share"}){
  rejected=false;try{materialRelativePathCandidate(name,".vtf");}catch(const std::runtime_error&){rejected=true;}check(rejected,"asset file policy remains strict");
 }
 auto adversarial=readSessionMaterialOverridesCandidate(identityGraph({(root/"outside.vmt").generic_string(),"../outside","C:/outside"}),0);
 check(adversarial.size()==3,"resource identities do not open files");auto noMatch=sessionMaterialBindingCandidate(adversarial,{mount});syntax=parseVmtCandidate("VertexLitGeneric { \"$basetexture\" \"fixture/base\" }");noMatch(vmt,syntax);
 check(vmtValuesCandidate(syntax,"$basetexture")==std::vector<std::string>{"fixture/base"},"adversarial resource names not converted to paths");
 ModelRenderInputsCandidate model;model.indices={0,1,2};model.ranges={{0,3,0}};model.materials.resize(2);
 for(auto&m:model.materials)m.source.syntax=parseVmtCandidate("VertexLitGeneric { \"$alpha\" \"0.9\" }");model.materials[0].source.vmt=vmt;model.materials[1].source.vmt=mount/"materials/models/b.vmt";
 SceneMaterialInputsCandidate inputs;inputs.entries={&model};inputs.alpha={{{"models/a",.4f},{"/models/a",.8f},{"/models/b",.2f}}};
 auto draw=prepareSceneMaterialDrawInputsCandidate(inputs,{mount});check(draw[0].alpha==std::vector<float>{.4f,.9f},"draw alpha matching preserves distinct resource keys");
 check(&draw[0].indices==&model.indices&&&draw[0].ranges==&model.ranges,"draw topology remains referenced");
 check(draw[0].materials[0].translucent&&draw[0].materials[1].translucent,"draw alpha translucency retained");
 std::ifstream sentinel(root/"outside.vmt");std::string content((std::istreambuf_iterator<char>(sentinel)),{});check(content=="DO NOT READ OR MODIFY","external fixture unchanged");
 std::cout<<"PASS material_identity_safety_checks="<<checks<<" filesystem_policy_unchanged=1 native_renderer=unverified\n";return 0;
}
int main(int argc,char**argv){try{
 if(argc==4&&std::string(argv[1])=="--audit")return auditIdentity(argv[2],argv[3]);
 if(argc==3&&std::string(argv[1])=="--self-test")return safetyIdentity(argv[2]);
 throw std::runtime_error("--audit session fresh-output OR --self-test fresh-workspace-directory");
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
