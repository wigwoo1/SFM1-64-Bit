#define NOMINMAX
#include "material_fallback_parameters_candidate.h"
#include "source_material_source_provider_candidate.h"
#include <iostream>
#include <sstream>
#include <map>
static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}
static uint32_t word(std::istream&in){uint32_t v=0;require(bool(in.read(reinterpret_cast<char*>(&v),4)),"Input word");return v;}
static std::string text(std::istream&in,unsigned maximum){auto n=word(in);require(n<=maximum,"Text extent");std::string s(n,'\0');require(bool(in.read(s.data(),n)),"Input text");return s;}
static SourceMaterialPatchNodeCandidate node(std::istream&in,unsigned depth=0){require(depth<16,"Input depth");SourceMaterialPatchNodeCandidate n;n.kind=word(in);n.bits=word(in);n.key=text(in,255);n.text=text(in,4095);auto count=word(in);require(count<256,"Input children");for(unsigned i=0;i<count;++i)n.children.push_back(node(in,depth+1));return n;}
static void put(std::ostream&out,uint32_t n){out.write(reinterpret_cast<const char*>(&n),4);}
static void putText(std::ostream&out,const std::string&s){put(out,uint32_t(s.size()));out.write(s.data(),s.size());}
static void tree(std::ostream&out,const SourceMaterialPatchNodeCandidate&n){putText(out,n.key);put(out,n.kind);put(out,n.bits);putText(out,n.text);out.write(reinterpret_cast<const char*>(&n.pointer),8);put(out,uint32_t(n.children.size()));for(const auto&c:n.children)tree(out,c);}
static void result(std::ostream&out,const MaterialFallbackParametersCandidate&r){
    const auto&m=r.material;put(out,m.parameters.warnings);put(out,m.selectedNode);putText(out,m.shader);put(out,uint32_t(m.lookups.size()));for(const auto&s:m.lookups)putText(out,s);tree(out,r.root);tree(out,r.aggregate);put(out,uint32_t(m.parameters.values.size()));
    for(const auto&v:m.parameters.values){std::array<uint32_t,28>q{};q[0]=v.created?1u:0u;q[1]=v.value.numeric.typeAndMetadata;q[2]=v.value.numeric.integer;std::copy(v.value.numeric.vector.begin(),v.value.numeric.vector.end(),q.begin()+3);q[7]=(q[1]&15)==5?1u:(q[1]&15)==8?2u:0u;q[8]=v.matrix?1u:0u;q[9]=v.matrixIdentity?1u:0u;q[10]=uint32_t(v.value.text.size());q[11]=uint32_t(v.name.size());if(v.matrix)std::copy(v.matrix->begin(),v.matrix->end(),q.begin()+12);out.write(reinterpret_cast<const char*>(q.data()),sizeof(q));out.write(v.value.text.data(),v.value.text.size());out.write(v.name.data(),v.name.size());}
}
static std::string snapshot(const MaterialFallbackParametersCandidate&r){std::ostringstream out;result(out,r);put(out,uint32_t(r.dependencies.size()));for(const auto&s:r.dependencies)putText(out,s);put(out,uint32_t(r.patchFiles.size()));for(const auto&s:r.patchFiles)putText(out,s);return out.str();}
static int safety(){
    using Node=SourceMaterialPatchNodeCandidate;unsigned checks=0,refusals=0;auto check=[&](bool b){if(!b)throw std::runtime_error("Fallback analytical check="+std::to_string(checks));++checks;};
    Node source;source.key="Wireframe";Node fallback;fallback.key="$fallbackmaterial";fallback.kind=1;fallback.text="target";source.children.push_back(fallback);Node error;error.key="Wireframe_DX9";error.children.push_back(Node{"errorMarker",1,"yes"});
    std::map<std::string,std::string>files{{"materials/target.vmt","VertexLitGeneric{$alpha .375}"}};
    auto provider=[&](const std::string&p)->std::optional<std::string>{auto i=files.find(p);return i==files.end()?std::nullopt:std::optional<std::string>(i->second);};
    std::ostringstream initialSource;tree(initialSource,source);tree(initialSource,error);const auto originalFiles=files;MaterialFallbackParametersCandidate out;
    check(materialFallbackParametersCandidate(out,source,error,"owner",false,provider));check(out.material.lookups==std::vector<std::string>{"Wireframe","VertexLitGeneric"}&&out.dependencies==std::vector<std::string>{"materials/target.vmt"}&&out.material.parameters.values[5].value.numeric.vector[0]==0x3ec00000);
    std::ostringstream afterSource;tree(afterSource,source);tree(afterSource,error);check(afterSource.str()==initialSource.str()&&files==originalFiles);const auto before=snapshot(out);
    for(unsigned kind=0;kind<8;++kind){auto bad=source;files=originalFiles;std::string owner="owner";
        if(kind==0)bad.children[0].text=std::string(201,'x');if(kind==1)bad.children[0].text="../../escape";if(kind==2)bad.children[0].kind=2;if(kind==3)bad.key="UnlitGeneric";if(kind==4)files["materials/target.vmt"]="patch{include ../../escape}";
        if(kind==5){files["materials/target.vmt"]="Wireframe{$fallbackmaterial again}";files["materials/again.vmt"]="Wireframe{$fallbackmaterial target}";}
        if(kind==6)files["materials/target.vmt"]="VertexLitGeneric{} Extra{}";
        if(kind==7)owner.clear();const auto input=files;check(!materialFallbackParametersCandidate(out,bad,error,owner,false,provider)&&snapshot(out)==before&&files==input);++refusals;
    }
    files.clear();MaterialFallbackParametersCandidate recovered;check(materialFallbackParametersCandidate(recovered,source,error,"owner",false,provider)&&recovered.material.parameters.warnings==1&&recovered.root.key=="Wireframe_DX9");
    auto ignored=source;ignored.key="VertexLitGeneric";check(materialFallbackParametersCandidate(recovered,ignored,error,"owner",false,provider)&&recovered.dependencies.empty()&&recovered.material.parameters.warnings==0);
    files=originalFiles;{auto temporary=files;auto ownedProvider=[temporary](const std::string&p)->std::optional<std::string>{auto i=temporary.find(p);return i==temporary.end()?std::nullopt:std::optional<std::string>(i->second);};check(materialFallbackParametersCandidate(recovered,source,error,"owner",false,ownedProvider));}check(snapshot(recovered)==before);
    MaterialParameterOrchestrationCandidate legacy;check(!materialParameterOrchestrationCandidate(legacy,source,{}, {},92));
    files=originalFiles;Node deep;deep.key="deep";for(unsigned i=0;i<18;++i){Node parent;parent.key="deep";parent.children.push_back(std::move(deep));deep=std::move(parent);}
    check(!materialFallbackParametersCandidate(out,deep,error,"owner",false,provider)&&snapshot(out)==before);++refusals;
    check(!materialFallbackParametersCandidate(out,source,deep,"owner",false,provider)&&snapshot(out)==before);++refusals;
    auto patch=source;files["materials/target.vmt"]="patch{include materials/base.vmt insert{$alpha .5}}";files["materials/base.vmt"]="VertexLitGeneric{$alpha 1}";
    check(materialFallbackParametersCandidate(recovered,patch,error,"owner",false,provider)&&recovered.dependencies==std::vector<std::string>{"materials/target.vmt","materials/base.vmt"}&&recovered.patchFiles==std::vector<std::string>{"materials/target.vmt"});
    files["materials/target.vmt"]="patch{include materials/a.vmt insert{firstMarker 1}}";files["materials/a.vmt"]="Wireframe{$fallbackmaterial second}";files["materials/second.vmt"]="patch{include materials/final.vmt insert{secondMarker 2}}";files["materials/final.vmt"]="VertexLitGeneric{}";
    check(materialFallbackParametersCandidate(recovered,source,error,"owner",false,provider)&&sourceMaterialPatchFindConstCandidate(recovered.aggregate,"insert/secondMarker")&&!sourceMaterialPatchFindConstCandidate(recovered.aggregate,"insert/firstMarker"));
    std::cout<<"PASS analytical_checks="<<checks<<" refusals="<<refusals<<'\n';return 0;
}
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"Owned fallback is x64");if(argc==2&&std::string(argv[1])=="--test")return safety();require(argc==5,"Mount input fresh output nonexistent engine path required");require(!std::filesystem::exists(argv[3])&&!std::filesystem::exists(argv[4]),"Fresh output/absent original runtime");
    SourceMaterialSourceProviderCandidate files({{std::filesystem::path(argv[1]),"GAME"}});auto provider=files.materialSource();std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);require(bool(in)&&bool(out),"Streams");const auto count=word(in);put(out,count);uint32_t developerMessages=0;
    for(unsigned i=0;i<count;++i){auto owner=text(in,255);const auto prefix=word(in);require(prefix<=1,"Prefix bool");auto source=node(in),error=node(in);MaterialFallbackParametersCandidate value;const auto fp=materialTrigX87EnvironmentCandidate();require(materialFallbackParametersCandidate(value,source,error,owner,prefix!=0,provider),"Owned fallback refusal");require(materialTrigX87EnvironmentCandidate()==fp,"FP state changed");developerMessages+=value.material.developerDiagnosticCalls;result(out,value);}
    require(in.peek()==std::char_traits<char>::eof(),"Trailing input");out.flush();require(bool(out),"Output failed");std::cout<<"PASS owned_fallback_cases="<<count<<" dev_messages="<<developerMessages<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
