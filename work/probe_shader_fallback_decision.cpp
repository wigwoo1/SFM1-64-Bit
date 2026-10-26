#include "shader_fallback_decision_candidate.h"
#include "material_parameter_collection_candidate.h"
#include "material_parameter_flags_candidate.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>
#ifdef SFM64_NATIVE_SHADER_FALLBACK
#define NOMINMAX
#include <windows.h>
#include <float.h>
struct CompletionAction{uint32_t index=0,operation=0;std::array<uint32_t,4>bits{};};
#include "shader_fallback_native_fixture.h"
static size_t activeCase=0;
static LONG CALLBACK observe(EXCEPTION_POINTERS*p){std::fprintf(stderr,"fallback_case=%zu code=%08lx eip=%08lx\n",activeCase,p->ExceptionRecord->ExceptionCode,p->ContextRecord->Eip);return EXCEPTION_CONTINUE_SEARCH;}
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static void write(std::ostream&out,const ShaderFallbackDecisionCandidate&d,uint32_t baseMetadata){
    const std::array<uint32_t,4>h{d.fallback?1u:0u,d.fallback?uint32_t(d.fallback->size()):0u,uint32_t(d.hardwareQueries.size()),baseMetadata};
    out.write(reinterpret_cast<const char*>(h.data()),sizeof(h));if(d.fallback)out.write(d.fallback->data(),d.fallback->size());out.write(reinterpret_cast<const char*>(d.hardwareQueries.data()),4*d.hardwareQueries.size());
}
#ifndef SFM64_NATIVE_SHADER_FALLBACK
static int analytical(){
    unsigned checks=0;auto check=[&](bool b){if(!b)throw std::runtime_error("Fallback analytical check="+std::to_string(checks));++checks;};
    auto index=[](std::string_view name){const auto&e=shaderFallbackEntriesCandidate();for(size_t i=0;i<e.size();++i)if(e[i].name==name)return i;throw std::runtime_error("Known shader name");};
    ShaderFallbackDecisionCandidate out;ShaderFallbackEnvironmentCandidate env;
    env.level17={90,91};check(shaderFallbackDecisionCandidate(out,index("ProtoIce_dx9"),env)&&out.fallback=="Wireframe"&&out.hardwareQueries==std::vector<uint32_t>{17,17});
    env.level17={89,100};check(shaderFallbackDecisionCandidate(out,index("Prototype_dx9"),env)&&out.hardwareQueries==std::vector<uint32_t>{17});
    env.level17={92,92};env.configByte38=0xff;check(shaderFallbackDecisionCandidate(out,index("ProtoIce_dx9"),env)&&out.fallback=="VertexLitGeneric");
    env.configPresent=false;check(shaderFallbackDecisionCandidate(out,index("ProtoIce_dx9"),env)&&!out.fallback);
    env.return34=0x100;check(shaderFallbackDecisionCandidate(out,index("Water_DX9_HDR"),env)&&!out.fallback);
    env.return34=0;check(shaderFallbackDecisionCandidate(out,index("Water_DX9_HDR"),env)&&out.fallback=="WATER_DX90");
    env.parametersPresent=false;check(shaderFallbackDecisionCandidate(out,index("MonitorScreen_DX9"),env)&&!out.fallback);
    env.parametersPresent=true;env.baseTypeByte=0xf6;check(shaderFallbackDecisionCandidate(out,index("MonitorScreen_DX9"),env)&&out.fallback=="LightmappedGeneric");
    env.baseTypeByte=0xff;check(shaderFallbackDecisionCandidate(out,index("MonitorScreen_DX9"),env)&&!out.fallback);
    check(shaderFallbackDecisionCandidate(out,index("VertexLitGeneric"),env)&&!out.fallback&&out.hardwareQueries.empty());
    auto snapshot=[&](){std::ostringstream s;write(s,out,0);return s.str();};const auto before=snapshot();unsigned refusals=0;
    for(size_t n:{size_t(157),size_t(-1)}){check(!shaderFallbackDecisionCandidate(out,n,env)&&snapshot()==before);++refusals;}
    std::cout<<"PASS analytical_checks="<<checks<<" refusals="<<refusals<<'\n';return 0;
}
#endif
int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_SHADER_FALLBACK
    if(argc==2&&std::string(argv[1])=="--test")return analytical();
#endif
    need(argc==5&&!std::ifstream(argv[4],std::ios::binary).good(),"Private libraries query fresh output required");
    std::ifstream in(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);need(bool(in)&&bool(out),"Fallback IO");
#ifdef SFM64_NATIVE_SHADER_FALLBACK
    static_assert(sizeof(void*)==4,"Private reference x86");ShaderFallbackNativeFixture native(argv[1],argv[2]);auto observer=AddVectoredExceptionHandler(1,&observe);need(observer!=nullptr,"Exception observer");
    unsigned oldControl=0,control=0;_controlfp_s(&oldControl,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
#else
    static_assert(sizeof(void*)==8,"Owned x64");
#endif
    const auto oldMxcsr=_mm_getcsr();_mm_setcsr(0x1f80);size_t cases=0;
    const std::vector<std::string>parameters={"$flags","$flags_defined","$flags2","$flags_defined2","$color","$alpha","$basetexture"};
    std::array<uint32_t,11>q{};
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
        need(q[0]<157&&q[5]<=1&&q[6]<=255&&q[7]<=1&&q[8]<=255&&q[8]!=5&&q[10]<=4095,"Fallback input domain");
        std::string value(q[10],'\0');need(bool(in.read(value.data(),value.size())),"Fallback text");
        ShaderFallbackEnvironmentCandidate env;std::memcpy(&env.level17[0],&q[1],4);std::memcpy(&env.level17[1],&q[2],4);std::memcpy(&env.capability24,&q[3],4);env.return34=q[4];env.configPresent=q[5]!=0;env.configByte38=uint8_t(q[6]);env.parametersPresent=q[7]!=0;
        ShaderFallbackDecisionCandidate result;uint32_t metadata=0xffffffff;
        MaterialParameterCollectionEnvironmentCandidate collectionEnv;
        const std::vector<MaterialParameterInputCandidate>base={{"$basetexture",value,q[8],q[9]}};
#ifdef SFM64_NATIVE_SHADER_FALLBACK
        activeCase=cases;std::array<void*,256>slots{};std::vector<std::array<uint8_t,40>>before;int count=0;
        if(env.parametersPresent){
            native.parameters=parameters;native.prepare(collectionEnv,base,{},true);std::array<uint8_t,32>root{};const uintptr_t child=reinterpret_cast<uintptr_t>(native.nodes[0].data());std::memcpy(root.data()+24,&child,4);
            using Collect=int(__thiscall*)(void*,void*,void*,void*,bool,void**);
            count=reinterpret_cast<Collect>(0x10005940)(&native.owner,&native.shader,root.data(),nullptr,false,slots.data());need(count==7,"Real fallback variable collection");
            for(int i=0;i<count;++i){before.emplace_back();std::memcpy(before.back().data(),slots[size_t(i)],40);}
            std::memcpy(&metadata,static_cast<uint8_t*>(slots[6])+28,4);metadata&=0xffff;
        }
        result=native.decide(q[0],env.parametersPresent?slots.data():nullptr,env);
        for(int i=0;i<count;++i){need(!std::memcmp(before[size_t(i)].data(),slots[size_t(i)],40),"Fallback changed real material variable");native.cleanup(slots[size_t(i)]);}
        need(*reinterpret_cast<uint32_t*>(native.image.base+0x10db14)==0,"Fallback pool leak");
#else
        if(env.parametersPresent){MaterialParameterCollectionCandidate collection;need(materialParameterCollectionCandidate(collection,parameters,materialParameterFlagNamesCandidate(),base,nullptr,collectionEnv),"Owned base construction");metadata=collection.values[6].value.numeric.typeAndMetadata;}
        env.baseTypeByte=uint8_t(metadata);need(shaderFallbackDecisionCandidate(result,q[0],env),"Owned fallback refusal");
#endif
        write(out,result,metadata);++cases;
    }
    out.flush();need(cases&&in.eof()&&in.gcount()==0&&bool(out),"Fallback framing");_mm_setcsr(oldMxcsr);
#ifdef SFM64_NATIVE_SHADER_FALLBACK
    native.finish();RemoveVectoredExceptionHandler(observer);_controlfp_s(&control,oldControl,_MCW_RC|_MCW_PC);std::cout<<"native_allocations="<<native.allocations<<" native_releases="<<native.releases<<'\n';
#endif
    std::cout<<"PASS fallback_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
