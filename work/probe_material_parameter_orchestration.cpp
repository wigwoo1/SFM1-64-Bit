#include "material_parameter_orchestration_candidate.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <iterator>
#include <cstring>
#include <stdexcept>
#ifdef SFM64_NATIVE_MATERIAL_ORCHESTRATION
#define NOMINMAX
#include <windows.h>
#include <float.h>
struct CompletionAction{uint32_t index=0,operation=0;std::array<uint32_t,4>bits{};};
#include "material_parameter_orchestration_native_fixture.h"
static size_t activeCase=0;
static LONG CALLBACK observe(EXCEPTION_POINTERS*p){std::fprintf(stderr,"loop_case=%zu exception=%08lx eip=%08lx address=%08lx\n",activeCase,p->ExceptionRecord->ExceptionCode,p->ContextRecord->Eip,p->ExceptionRecord->NumberParameters>1?static_cast<unsigned long>(p->ExceptionRecord->ExceptionInformation[1]):0ul);return EXCEPTION_CONTINUE_SEARCH;}
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t word(std::istream&in){uint32_t n=0;need(bool(in.read(reinterpret_cast<char*>(&n),4)),"Loop input word");return n;}
static std::string text(std::istream&in,size_t limit){auto n=word(in);need(n<=limit,"Loop text limit");std::string s(n,'\0');need(bool(in.read(s.data(),n)),"Loop text input");return s;}
static SourceMaterialPatchNodeCandidate node(std::istream&in,unsigned depth,unsigned&count){need(depth<=16&&++count<=4096,"Loop tree extent");SourceMaterialPatchNodeCandidate n;n.kind=word(in);n.bits=word(in);n.key=text(in,255);n.text=text(in,4095);auto c=word(in);need(c<=4096,"Loop children");for(uint32_t i=0;i<c;++i)n.children.push_back(node(in,depth+1,count));return n;}
static void write(std::ostream&out,const MaterialParameterCollectionCandidate&result){
    const std::array<uint32_t,2>head{uint32_t(result.values.size()),result.warnings};out.write(reinterpret_cast<const char*>(head.data()),sizeof(head));
    for(const auto&v:result.values){
        std::array<uint32_t,28>record{};record[0]=v.created?1u:0u;record[1]=v.value.numeric.typeAndMetadata;record[2]=v.value.numeric.integer;std::copy(v.value.numeric.vector.begin(),v.value.numeric.vector.end(),record.begin()+3);
        record[7]=(v.value.numeric.typeAndMetadata&15)==5?1u:(v.value.numeric.typeAndMetadata&15)==8?2u:0u;record[8]=v.matrix?1u:0u;record[9]=v.matrixIdentity?1u:0u;record[10]=uint32_t(v.value.text.size());record[11]=uint32_t(v.name.size());if(v.matrix)std::copy(v.matrix->begin(),v.matrix->end(),record.begin()+12);
        out.write(reinterpret_cast<const char*>(record.data()),sizeof(record));out.write(v.value.text.data(),v.value.text.size());out.write(v.name.data(),v.name.size());
    }
}
static void writeResult(std::ostream&out,const MaterialParameterOrchestrationCandidate&r){
    const std::array<uint32_t,3>h{r.selectedNode,uint32_t(r.shader.size()),uint32_t(r.lookups.size())};out.write(reinterpret_cast<const char*>(h.data()),sizeof(h));out.write(r.shader.data(),r.shader.size());
    for(const auto&s:r.lookups){const uint32_t n=uint32_t(s.size());out.write(reinterpret_cast<const char*>(&n),4);out.write(s.data(),s.size());}write(out,r.parameters);
}
#ifndef SFM64_NATIVE_MATERIAL_ORCHESTRATION
static void putText(std::ostream&out,const std::string&s){const auto n=uint32_t(s.size());out.write(reinterpret_cast<const char*>(&n),4);out.write(s.data(),s.size());}
static void putNode(std::ostream&out,const SourceMaterialPatchNodeCandidate&n){const std::array<uint32_t,2>h{n.kind,n.bits};out.write(reinterpret_cast<const char*>(h.data()),8);putText(out,n.key);putText(out,n.text);const uint32_t c=uint32_t(n.children.size());out.write(reinterpret_cast<const char*>(&c),4);for(const auto&child:n.children)putNode(out,child);}
static int filesMode(const char*input,const char*output,const char*projection){
    need(!std::ifstream(output,std::ios::binary).good()&&!std::ifstream(projection,std::ios::binary).good(),"Fresh file-composition outputs");std::ifstream in(input,std::ios::binary);std::ofstream out(output,std::ios::binary),projected(projection,std::ios::binary);need(bool(in)&&bool(out)&&bool(projected),"File composition IO");std::array<uint32_t,12>q{};size_t cases=0;
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
        const auto n=word(in);need(n<=256,"File catalog size");std::map<std::string,std::string>catalog;for(uint32_t i=0;i<n;++i){auto path=text(in,511),source=text(in,1u<<24);need(catalog.emplace(sourceMaterialKeyValuesPathCandidate(path),std::move(source)).second,"Duplicate catalog path");}auto path=text(in,511);
        MaterialParameterCollectionEnvironmentCandidate env;std::memcpy(&env.condition.gpu,&q[0],4);std::memcpy(&env.condition.hdr,&q[1],4);env.condition.srgbReturn=q[2];std::memcpy(&env.condition.lowFill,&q[3],4);env.percentGateReturn=q[4];env.percentKeepReturn=q[5];std::memcpy(&env.percentOverride,&q[6],4);VertexLitInitSettingsCandidate settings;settings.configFlagsAt34=q[7];std::memcpy(&settings.matPhong,&q[8],4);std::memcpy(&settings.forceVertexFog,&q[9],4);int32_t level=0;std::memcpy(&level,&q[10],4);need(q[11]<=1,"Tools bool");settings.toolsSwitchPresent=q[11]!=0;
        auto provider=[&](const std::string&p)->std::optional<std::string>{auto i=catalog.find(p);return i==catalog.end()?std::nullopt:std::optional<std::string>(i->second);};SourceMaterialParameterPipelineCandidate result;
        need(sourceMaterialParameterPipelineCandidate(result,path,provider,env,settings,level),"File parameter pipeline refusal");const uint32_t dependencies=uint32_t(result.dependencies.size());out.write(reinterpret_cast<const char*>(&dependencies),4);for(const auto&d:result.dependencies)putText(out,d);writeResult(out,result.material);
        auto resolved=sourceMaterialKeyValuesFilesCandidate(path,provider);need(!resolved.document.roots.empty(),"Projected first root");projected.write(reinterpret_cast<const char*>(q.data()),sizeof(q));putNode(projected,resolved.document.roots[0]);++cases;
    }
    out.flush();projected.flush();need(cases&&in.eof()&&!in.gcount()&&bool(out)&&bool(projected),"File composition framing");std::cout<<"PASS file_composition_cases="<<cases<<'\n';return 0;
}
static int analytical(){
    unsigned checks=0,refusals=0;auto check=[&](bool b){if(!b)throw std::runtime_error("Loop analytical check="+std::to_string(checks));++checks;};
    SourceMaterialPatchNodeCandidate root;root.key="Wireframe";MaterialParameterOrchestrationCandidate out;check(materialParameterOrchestrationCandidate(out,root,{}, {},92)&&out.shader=="Wireframe_DX9"&&out.lookups.size()==2);
    auto snapshot=[&](){std::ostringstream s;writeResult(s,out);return s.str();};const auto before=snapshot();
    for(unsigned kind=0;kind<4;++kind){auto bad=root;if(kind==0)bad.key="UnlitGeneric";if(kind==1)bad.key=std::string(256,'x');if(kind==2){SourceMaterialPatchNodeCandidate n;n.key="$fallbackmaterial";n.kind=1;n.text="fixture/other";bad.children.push_back(n);}if(kind==3){SourceMaterialPatchNodeCandidate n;n.key="bad";n.kind=5;bad.children.push_back(n);}check(!materialParameterOrchestrationCandidate(out,bad,{}, {},92)&&snapshot()==before);++refusals;}
    auto large=root;large.key="VertexLitGeneric";for(unsigned i=0;i<131;++i){SourceMaterialPatchNodeCandidate n;n.key="extra"+std::to_string(i);n.kind=2;n.bits=i;large.children.push_back(n);}check(!materialParameterOrchestrationCandidate(out,large,{}, {},92)&&snapshot()==before);++refusals;
    SourceMaterialParameterPipelineCandidate pipeline;std::map<std::string,std::string>files{{"root.vmt","#base base.vmt Wireframe{}"},{"base.vmt","Base{$model 1}"}};
    auto provider=[&](const std::string&p)->std::optional<std::string>{auto i=files.find(p);return i==files.end()?std::nullopt:std::optional<std::string>(i->second);};
    check(sourceMaterialParameterPipelineCandidate(pipeline,"root.vmt",provider,{}, {},92)&&pipeline.material.shader=="Wireframe_DX9"&&(pipeline.material.parameters.values[0].value.numeric.integer&0x800)&&pipeline.dependencies.size()==2);
    auto fileSnapshot=[&](){std::ostringstream s;writeResult(s,pipeline.material);for(const auto&d:pipeline.dependencies)putText(s,d);return s.str();};const auto pbefore=fileSnapshot();
    check(!sourceMaterialParameterPipelineCandidate(pipeline,"missing.vmt",provider,{}, {},92)&&fileSnapshot()==pbefore);++refusals;
    for(const auto&source:std::vector<std::string>{"patch{include base.vmt}","Wireframe{$fallbackmaterial other}","#include root.vmt Wireframe{}"}){
        files["root.vmt"]=source;check(!sourceMaterialParameterPipelineCandidate(pipeline,"root.vmt",provider,{}, {},92)&&fileSnapshot()==pbefore);++refusals;
    }
    std::cout<<"PASS analytical_checks="<<checks<<" refusals="<<refusals<<'\n';return 0;
}
#endif
int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_MATERIAL_ORCHESTRATION
    if(argc==2&&std::string(argv[1])=="--test")return analytical();
    if(argc==5&&std::string(argv[1])=="--files")return filesMode(argv[2],argv[3],argv[4]);
#endif
    need(argc==5&&!std::ifstream(argv[4],std::ios::binary).good(),"Loop private libraries query fresh output required");std::ifstream in(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);need(bool(in)&&bool(out),"Loop IO");
#ifdef SFM64_NATIVE_MATERIAL_ORCHESTRATION
    static_assert(sizeof(void*)==4,"Private loop x86");MaterialParameterOrchestrationNativeFixture native(argv[1],argv[2]);auto observer=AddVectoredExceptionHandler(1,&observe);need(observer!=nullptr,"Observer");unsigned oldControl=0,control=0;_controlfp_s(&oldControl,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
#else
    static_assert(sizeof(void*)==8,"Owned loop x64");
#endif
    auto oldMxcsr=_mm_getcsr();_mm_setcsr(0x1f80);std::array<uint32_t,12>q{};size_t cases=0;
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
        unsigned count=0;auto root=node(in,0,count);MaterialParameterCollectionEnvironmentCandidate env;std::memcpy(&env.condition.gpu,&q[0],4);std::memcpy(&env.condition.hdr,&q[1],4);env.condition.srgbReturn=q[2];std::memcpy(&env.condition.lowFill,&q[3],4);env.percentGateReturn=q[4];env.percentKeepReturn=q[5];std::memcpy(&env.percentOverride,&q[6],4);
        VertexLitInitSettingsCandidate settings;settings.configFlagsAt34=q[7];std::memcpy(&settings.matPhong,&q[8],4);std::memcpy(&settings.forceVertexFog,&q[9],4);int32_t level=0;std::memcpy(&level,&q[10],4);need(q[11]<=1,"Tools boolean");settings.toolsSwitchPresent=q[11]!=0;
        MaterialParameterOrchestrationCandidate result;
#ifdef SFM64_NATIVE_MATERIAL_ORCHESTRATION
        activeCase=cases;auto*selected=native.run(root,env,settings,level);need(selected!=nullptr,"Original loop failed");
        bool found=false;for(size_t i=0;i<native.tree.size();++i)if(selected==native.tree[i].data()){result.selectedNode=uint32_t(i);found=true;break;}need(found,"Selected original node");
        uintptr_t shader=0,array=0;std::memcpy(&shader,native.material.data()+0x14,4);std::memcpy(&array,native.material.data()+0x24,4);need(shader&&array&&native.material[0x22],"Real original final material fields");
        for(size_t i=0;i<native.shaders.size();++i)if(reinterpret_cast<uintptr_t>(native.shaders[i])==shader)result.shader=native.shaderNames[i];need(!result.shader.empty(),"Real selected shader identity");result.lookups=native.lookups;result.parameters.warnings=native.warnings+native.shaderWarnings;
        for(unsigned i=0;i<native.material[0x22];++i){auto*b=static_cast<uint8_t*>(reinterpret_cast<void**>(array)[i]);MaterialTypedDispatchCandidate v;v.created=true;uintptr_t owner=0,data=0;std::memcpy(&owner,b+32,4);need(owner==reinterpret_cast<uintptr_t>(native.material.data()),"Real material variable owner");
            std::memcpy(&v.value.numeric.typeAndMetadata,b+28,4);v.value.numeric.typeAndMetadata&=0xffff;std::memcpy(&v.value.numeric.integer,b+8,4);std::memcpy(v.value.numeric.vector.data(),b+12,16);using Name=const char*(__thiscall*)(void*);v.name=reinterpret_cast<Name>(0x10017b80)(b);
            if((v.value.numeric.typeAndMetadata&15)==1){std::memcpy(&data,b+4,4);v.value.text=reinterpret_cast<const char*>(data);}if((v.value.numeric.typeAndMetadata&15)==7){std::memcpy(&data,b+36,4);need(data!=0,"Matrix payload");v.matrix.emplace();std::memcpy(v.matrix->data(),reinterpret_cast<void*>(data),64);v.matrixIdentity=*reinterpret_cast<uint8_t*>(data+64)!=0;}result.parameters.values.push_back(std::move(v));
        }
        native.cleanupResult();
#else
        const auto beforeEnvironment=materialTrigX87EnvironmentCandidate();need(materialParameterOrchestrationCandidate(result,root,env,settings,level),"Owned orchestration refusal");need(materialTrigX87EnvironmentCandidate()==beforeEnvironment,"Owned FP environment changed");
#endif
        writeResult(out,result);++cases;
    }
    out.flush();need(cases&&in.eof()&&in.gcount()==0&&bool(out),"Loop framing");_mm_setcsr(oldMxcsr);
#ifdef SFM64_NATIVE_MATERIAL_ORCHESTRATION
    native.finish();RemoveVectoredExceptionHandler(observer);_controlfp_s(&control,oldControl,_MCW_RC|_MCW_PC);std::cout<<"native_allocations="<<native.allocations<<" native_releases="<<native.releases<<'\n';
#endif
    std::cout<<"PASS orchestration_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
