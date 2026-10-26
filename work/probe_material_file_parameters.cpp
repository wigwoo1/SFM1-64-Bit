#define NOMINMAX
#include "material_file_parameters_candidate.h"
#include "source_material_source_provider_candidate.h"
#include <iostream>
#include <sstream>
#include <map>

static void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}
static uint32_t word(std::istream&in){uint32_t n=0;require(bool(in.read(reinterpret_cast<char*>(&n),4)),"Input word");return n;}
static std::string text(std::istream&in,size_t maximum){const auto n=word(in);require(n<=maximum,"Input string extent");std::string s(n,'\0');require(bool(in.read(s.data(),n)),"Input text");return s;}
static void put(std::ostream&out,uint32_t n){out.write(reinterpret_cast<const char*>(&n),4);}
static void putText(std::ostream&out,const std::string&s){put(out,uint32_t(s.size()));out.write(s.data(),s.size());}
// Same explicit raw-document and active-value records as the accepted probes.
static void rawNode(std::ostream&out,const SourceMaterialPatchNodeCandidate&n){
    putText(out,n.key);put(out,n.kind);
    if(n.kind==1)putText(out,n.text);else if(n.kind==2||n.kind==3)put(out,n.bits);
    else if(n.kind==4||n.kind==7)out.write(reinterpret_cast<const char*>(&n.pointer),8);
    put(out,uint32_t(n.children.size()));for(const auto&c:n.children)rawNode(out,c);
}
static void rawDocument(std::ostream&out,const SourceMaterialPatchFilesResultCandidate&r){put(out,1);put(out,uint32_t(r.document.roots.size()));for(const auto&n:r.document.roots)rawNode(out,n);rawNode(out,r.aggregate);}
static void projectedNode(std::ostream&out,const SourceMaterialPatchNodeCandidate&n){put(out,n.kind);put(out,n.bits);putText(out,n.key);putText(out,n.text);put(out,uint32_t(n.children.size()));for(const auto&c:n.children)projectedNode(out,c);}
static void parameters(std::ostream&out,const MaterialParameterOrchestrationCandidate&r){
    put(out,r.selectedNode);put(out,uint32_t(r.shader.size()));put(out,uint32_t(r.lookups.size()));out.write(r.shader.data(),r.shader.size());for(const auto&s:r.lookups)putText(out,s);
    put(out,uint32_t(r.parameters.values.size()));put(out,r.parameters.warnings);
    for(const auto&v:r.parameters.values){
        std::array<uint32_t,28>q{};q[0]=v.created?1u:0u;q[1]=v.value.numeric.typeAndMetadata;q[2]=v.value.numeric.integer;std::copy(v.value.numeric.vector.begin(),v.value.numeric.vector.end(),q.begin()+3);
        q[7]=(q[1]&15)==5?1u:(q[1]&15)==8?2u:0u;q[8]=v.matrix?1u:0u;q[9]=v.matrixIdentity?1u:0u;q[10]=uint32_t(v.value.text.size());q[11]=uint32_t(v.name.size());if(v.matrix)std::copy(v.matrix->begin(),v.matrix->end(),q.begin()+12);
        out.write(reinterpret_cast<const char*>(q.data()),sizeof(q));out.write(v.value.text.data(),v.value.text.size());out.write(v.name.data(),v.name.size());
    }
}
static void names(std::ostream&out,const std::vector<std::string>&v){put(out,uint32_t(v.size()));for(const auto&s:v)putText(out,s);}
static std::string snapshot(const MaterialFileParametersCandidate&r){std::ostringstream out;rawDocument(out,r.source);names(out,r.source.dependencies);names(out,r.source.patchFiles);parameters(out,r.material);return out.str();}
static int safety(){
    using Catalog=std::map<std::string,std::string>;Catalog files{
        {"materials/root.vmt","patch{include materials/inner.vmt insert{$alpha .25} replace{$model 0}}"},
        {"materials/inner.vmt","patch{include materials/base.vmt insert{$alpha .5} replace{$model 1}}"},
        {"materials/base.vmt","Wireframe{$model 0 $alpha 1}"}};
    auto provider=[&](const std::string&p)->std::optional<std::string>{auto i=files.find(p);return i==files.end()?std::nullopt:std::optional<std::string>(i->second);};
    unsigned checks=0,refusals=0;auto check=[&](bool v){if(!v)throw std::runtime_error("File parameter safety assertion index="+std::to_string(checks));++checks;};MaterialFileParametersCandidate out;
    check(materialFileParametersCandidate(out,"root",provider,{}, {},92));
    check(out.material.shader=="Wireframe_DX9"&&out.material.lookups==std::vector<std::string>{"Wireframe","Wireframe_DX9"}&&out.material.parameters.values[5].value.numeric.vector[0]==0x3f000000u);
    check(out.source.dependencies==std::vector<std::string>{"materials/root.vmt","materials/inner.vmt","materials/base.vmt"}&&out.source.patchFiles.size()==2);
    const auto before=snapshot(out);const auto originalFiles=files;
    {files["materials/root.vmt"]="patch{include materials/base.vmt insert{\"valid/path\" 1}}";MaterialFileParametersCandidate pathResult;check(materialFileParametersCandidate(pathResult,"root",provider,{}, {},92)&&sourceMaterialPatchFindConstCandidate(pathResult.source.document.roots.front(),"valid/path")->bits==1);}
    for(const auto&source:std::vector<std::string>{"patch{}","patch{include materials/missing.vmt}","patch{include materials/root.vmt}","patch{include ../../escape}","patch{include materials/base.vmt insert{$fallbackmaterial other}}","patch{include materials/base.vmt insert{\""+std::string(256,'x')+"\" 1}}","LightmappedGeneric{}","Wireframe{bad}","#include root.vmt Wireframe{}"}){
        files["materials/root.vmt"]=source;const auto input=files;check(!materialFileParametersCandidate(out,"root",provider,{}, {},92)&&snapshot(out)==before&&files==input);++refusals;
    }
    files=originalFiles;for(const auto&name:std::vector<std::string>{"missing","../escape","{procedural}"}){check(!materialFileParametersCandidate(out,name,provider,{}, {},92)&&snapshot(out)==before&&files==originalFiles);++refusals;}
    {MaterialFileParametersCandidate temporary;check(materialFileParametersCandidate(temporary,"root",provider,{}, {},92));files.clear();check(snapshot(temporary)==before);}
    std::cout<<"PASS analytical_checks="<<checks<<" refusals="<<refusals<<'\n';return 0;
}
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"Owned file parameter pipeline is x64");
    if(argc==2&&std::string(argv[1])=="--test")return safety();
    require(argc==7,"Input, fresh raw/parameters/projection/metadata outputs and nonexistent engine path required");
    require(!std::filesystem::exists(argv[6]),"Owned engine-independence marker must not exist");
    for(int i=2;i<=5;++i)require(!std::filesystem::exists(argv[i]),"Fresh output required");
    std::ifstream in(argv[1],std::ios::binary);std::ofstream raw(argv[2],std::ios::binary),out(argv[3],std::ios::binary),projection(argv[4],std::ios::binary),metadata(argv[5],std::ios::binary);require(bool(in)&&bool(raw)&&bool(out)&&bool(projection)&&bool(metadata),"File IO");
    const auto n=word(in);require(n&&n<=128,"Mount extent");std::vector<SourceMaterialSourceMountCandidate>mounts;
    for(uint32_t i=0;i<n;++i){auto path=text(in,32767),id=text(in,64);const auto position=word(in);require(position<=1,"Mount position");SourceMaterialSourceMountCandidate m{std::filesystem::path(path),id};if(position)mounts.push_back(std::move(m));else mounts.insert(mounts.begin(),std::move(m));}
    SourceMaterialSourceProviderCandidate files(mounts);const auto provider=files.materialSource();const auto count=word(in);require(count&&count<=100000,"Case extent");put(raw,count);
    for(uint32_t i=0;i<count;++i){auto name=text(in,511);const auto prefix=word(in);require(prefix<=1,"Root prefix");std::array<uint32_t,12>q{};require(bool(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))),"State header");
        MaterialParameterCollectionEnvironmentCandidate env;std::memcpy(&env.condition.gpu,&q[0],4);std::memcpy(&env.condition.hdr,&q[1],4);env.condition.srgbReturn=q[2];std::memcpy(&env.condition.lowFill,&q[3],4);env.percentGateReturn=q[4];env.percentKeepReturn=q[5];std::memcpy(&env.percentOverride,&q[6],4);
        VertexLitInitSettingsCandidate settings;settings.configFlagsAt34=q[7];std::memcpy(&settings.matPhong,&q[8],4);std::memcpy(&settings.forceVertexFog,&q[9],4);int32_t hardware=0;std::memcpy(&hardware,&q[10],4);require(q[11]<=1,"Tools boolean");settings.toolsSwitchPresent=q[11]!=0;
        const auto fp=materialTrigX87EnvironmentCandidate();MaterialFileParametersCandidate result;require(materialFileParametersCandidate(result,name,provider,env,settings,hardware,bool(prefix)),"Owned file parameter refusal");require(materialTrigX87EnvironmentCandidate()==fp,"Owned FP state changed");
        rawDocument(raw,result.source);parameters(out,result.material);projection.write(reinterpret_cast<const char*>(q.data()),sizeof(q));projectedNode(projection,result.source.document.roots.front());names(metadata,result.source.dependencies);names(metadata,result.source.patchFiles);
    }
    require(in.peek()==std::char_traits<char>::eof(),"Trailing input");raw.flush();out.flush();projection.flush();metadata.flush();require(bool(raw)&&bool(out)&&bool(projection)&&bool(metadata),"Output failed");std::cout<<"PASS material_file_parameter_cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
