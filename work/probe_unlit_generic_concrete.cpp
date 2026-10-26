#ifdef SFM64_NATIVE_UNLIT_CONCRETE
#include "vertexlit_concrete_init_candidate.h"
#else
#include "unlit_generic_init_candidate.h"
#endif
struct CompletionAction{uint32_t index=0,operation=0;std::array<uint32_t,4>bits{};};
#include "material_parameter_flags_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <cstring>
#include <sstream>
#ifdef SFM64_NATIVE_UNLIT_CONCRETE
#define NOMINMAX
#include <windows.h>
#include <float.h>
#include "unlit_generic_native_fixture.h"
static size_t activeCase=0;
static LONG CALLBACK exceptionObserver(EXCEPTION_POINTERS*p){std::fprintf(stderr,"collection_case=%zu code=%08lx eip=%08lx\n",activeCase,p->ExceptionRecord->ExceptionCode,p->ContextRecord->Eip);return EXCEPTION_CONTINUE_SEARCH;}
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t word(std::istream&in){uint32_t v=0;need(bool(in.read(reinterpret_cast<char*>(&v),4)),"Collection input word");return v;}
static std::string text(std::istream&in,size_t limit){const auto n=word(in);need(n<=limit,"Collection text extent");std::string s(n,'\0');need(bool(in.read(s.data(),n)),"Collection text input");return s;}
static void write(std::ostream&out,const MaterialParameterCollectionCandidate&result){
    const std::array<uint32_t,2>head{uint32_t(result.values.size()),result.warnings};out.write(reinterpret_cast<const char*>(head.data()),sizeof(head));
    for(const auto&v:result.values){
        std::array<uint32_t,28>record{};record[0]=v.created?1u:0u;record[1]=v.value.numeric.typeAndMetadata;record[2]=v.value.numeric.integer;std::copy(v.value.numeric.vector.begin(),v.value.numeric.vector.end(),record.begin()+3);
        record[7]=(v.value.numeric.typeAndMetadata&15)==5?1u:(v.value.numeric.typeAndMetadata&15)==8?2u:0u;record[8]=v.matrix?1u:0u;record[9]=v.matrixIdentity?1u:0u;record[10]=uint32_t(v.value.text.size());record[11]=uint32_t(v.name.size());if(v.matrix)std::copy(v.matrix->begin(),v.matrix->end(),record.begin()+12);
        out.write(reinterpret_cast<const char*>(record.data()),sizeof(record));out.write(v.value.text.data(),v.value.text.size());out.write(v.name.data(),v.name.size());
    }
}
#ifndef SFM64_NATIVE_UNLIT_CONCRETE
static int analytical();

#endif
int main(int argc,char**argv){try{
#ifdef SFM64_NATIVE_UNLIT_CONCRETE
    if(argc==5&&std::string(argv[1])=="--metadata"){
        need(!std::ifstream(argv[4],std::ios::binary).good(),"Fresh schema output");
        UnlitGenericNativeFixture native(argv[2],argv[3]);std::ofstream out(argv[4],std::ios::binary);need(bool(out),"Schema output");
        auto put=[&](uint32_t n){out.write(reinterpret_cast<const char*>(&n),4);};put(68);
        for(size_t i=0;i<68;++i){put(uint32_t(native.parameters[i].size()));out.write(native.parameters[i].data(),native.parameters[i].size());put(native.types[i]);}
        put(uint32_t(native.mapping.size()));for(auto i:native.mapping)put(uint32_t(i));out.flush();need(bool(out),"Schema completion");native.finish();
        std::cout<<"PASS actual_unlit_schema=68 mapping_fields=110\n";return 0;
    }
#endif
#ifndef SFM64_NATIVE_UNLIT_CONCRETE
    if(argc==2&&std::string(argv[1])=="--test")return analytical();
    if(argc==3&&std::string(argv[1])=="--metadata"){
        need(!std::ifstream(argv[2],std::ios::binary).good(),"Fresh owned schema output");std::ofstream out(argv[2],std::ios::binary);need(bool(out),"Owned schema stream");
        auto put=[&](uint32_t n){out.write(reinterpret_cast<const char*>(&n),4);};put(68);const auto types=unlitGenericParameterTypesCandidate();
        for(size_t i=0;i<68;++i){const auto name=unlitGenericParameterNamesCandidate()[i];put(uint32_t(name.size()));out.write(name.data(),name.size());put(types[i]);}
        out.flush();need(bool(out),"Owned schema output");std::cout<<"PASS owned_unlit_schema=68\n";return 0;
    }
#endif
    need(argc==5&&!std::ifstream(argv[4],std::ios::binary).good(),"Library query fresh-output required");
    std::ifstream in(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);need(bool(in)&&bool(out),"Collection IO");
#ifdef SFM64_NATIVE_UNLIT_CONCRETE
    static_assert(sizeof(void*)==4,"Private reference x86");UnlitGenericNativeFixture native(argv[1],argv[2]);auto observer=AddVectoredExceptionHandler(1,&exceptionObserver);need(observer!=nullptr,"Exception observer");
    unsigned oldControl=0,control=0;_controlfp_s(&oldControl,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);size_t notifications=0;
#else
    static_assert(sizeof(void*)==8,"Owned x64");
#endif
    const auto oldMxcsr=_mm_getcsr();_mm_setcsr(0x1f80);size_t cases=0;std::array<uint32_t,12>q{};
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
        need(q[0]<=31&&q[8]==68&&q[9]<=32&&q[10]+q[11]<=512,"Collection input counts");
        // Conservative native bound: even if every supplied node appends an
        // extra, the original unchecked 256-entry output table cannot overflow.
        need(q[8]+q[10]+q[11]<=256,"Safe native output domain");
        MaterialParameterCollectionEnvironmentCandidate env;env.model=(q[0]&1)!=0;env.numericSse2=(q[0]&2)!=0;env.trigSse2=(q[0]&4)!=0;
        std::memcpy(&env.condition.lowFill,&q[1],4);std::memcpy(&env.condition.hdr,&q[2],4);env.condition.srgbReturn=q[3];std::memcpy(&env.condition.gpu,&q[4],4);env.percentGateReturn=q[5];env.percentKeepReturn=q[6];std::memcpy(&env.percentOverride,&q[7],4);
        std::vector<std::string>parameters,flagNames;for(uint32_t i=0;i<q[8];++i)parameters.push_back(text(in,1023));for(uint32_t i=0;i<q[9];++i)flagNames.push_back(text(in,1023));
        std::vector<MaterialParameterInputCandidate>base,over;
        for(uint32_t i=0;i<q[10]+q[11];++i){MaterialParameterInputCandidate n;n.type=word(in);n.bits=word(in);n.name=text(in,1023);n.text=text(in,4095);need(n.type<=255&&n.type!=5,"Native type domain");const auto pos=n.name.find('?');need(pos==std::string::npos||pos<=255,"Native prefix domain");(i<q[10]?base:over).push_back(std::move(n));}
        const auto repeats=word(in);need(repeats>=1&&repeats<=3,"Stage repeats");
        VertexLitInitSettingsCandidate settings;settings.configFlagsAt34=word(in);const auto matPhong=word(in),forceFog=word(in),levelWord=word(in),toolsWord=word(in);int32_t hardwareLevel=0;
        std::memcpy(&settings.matPhong,&matPhong,4);std::memcpy(&settings.forceVertexFog,&forceFog,4);std::memcpy(&hardwareLevel,&levelWord,4);need(toolsWord<=1,"Tools boolean");settings.toolsSwitchPresent=toolsWord!=0;
        #ifdef SFM64_NATIVE_UNLIT_CONCRETE
        const auto types=native.types;need(parameters==native.parameters,"Actual registered schema");
#else
        const auto types=unlitGenericParameterTypesCandidate();const auto&registered=unlitGenericParameterNamesCandidate();for(size_t i=0;i<68;++i)need(parameters[i]==registered[i],"Fixed registered schema");
#endif
        uint32_t route=0;
        std::vector<uint32_t>lookups;MaterialParameterCollectionCandidate result;
#ifdef SFM64_NATIVE_UNLIT_CONCRETE
        activeCase=cases;need(native.parameters==parameters&&native.types==types,"Actual shader descriptor identity");native.flags=flagNames;native.prepare(env,base,over,(q[0]&16)!=0);
        std::array<uint8_t,32>baseRoot{},overRoot{};const uintptr_t baseChild=base.empty()?0:reinterpret_cast<uintptr_t>(native.nodes[0].data()),overChild=over.empty()?0:reinterpret_cast<uintptr_t>(native.nodes[base.size()].data());std::memcpy(baseRoot.data()+24,&baseChild,4);std::memcpy(overRoot.data()+24,&overChild,4);
        std::array<void*,258>slots{};slots.front()=reinterpret_cast<void*>(0x11223344);slots.back()=reinterpret_cast<void*>(0x55667788);
        using Collect=int(__thiscall*)(void*,void*,void*,void*,bool,void**);
        const int count=reinterpret_cast<Collect>(0x10005940)(&native.owner,native.shaderObject,baseRoot.data(),(q[0]&8)?overRoot.data():nullptr,env.model,slots.data()+1);
        need(count>=4&&count<=256&&slots.front()==reinterpret_cast<void*>(0x11223344)&&slots.back()==reinterpret_cast<void*>(0x55667788),"Native result bounds/guards");
        result.warnings=native.warnings;
        for(uint32_t repeat=0;repeat<repeats;++repeat){route=native.completeActual(slots.data()+1,settings,hardwareLevel);result.warnings+=native.shaderWarnings;}
        need(native.warnings<=result.warnings,"Diagnostic count");
        for(int i=0;i<count;++i){
            auto*b=static_cast<uint8_t*>(slots[size_t(i)+1]);need(b!=nullptr,"Native populated slot");MaterialTypedDispatchCandidate v;v.created=true;
            uintptr_t owner=0;std::memcpy(&owner,b+32,4);need(owner==reinterpret_cast<uintptr_t>(&native.owner),"Native material ownership");
            std::memcpy(&v.value.numeric.typeAndMetadata,b+28,4);v.value.numeric.typeAndMetadata&=0xffffu;std::memcpy(&v.value.numeric.integer,b+8,4);std::memcpy(v.value.numeric.vector.data(),b+12,16);
            using Name=const char*(__thiscall*)(void*);v.name=reinterpret_cast<Name>(0x10017b80)(b);uintptr_t data=0;
            if((v.value.numeric.typeAndMetadata&15)==1){std::memcpy(&data,b+4,4);v.value.text=reinterpret_cast<const char*>(data);}
            if((v.value.numeric.typeAndMetadata&15)==7){std::memcpy(&data,b+36,4);need(data!=0,"Native matrix resource");v.matrix.emplace();std::memcpy(v.matrix->data(),reinterpret_cast<void*>(data),64);v.matrixIdentity=*reinterpret_cast<uint8_t*>(data+64)!=0;}
            const auto tag=v.value.numeric.typeAndMetadata&15;
            if(tag==5){std::memcpy(&data,b+36,4);need(data&&!*reinterpret_cast<uint32_t*>(data)&&!*reinterpret_cast<uint32_t*>(data+4),"Native FourCC zero/null payload");}
            if(tag==8){std::memcpy(&data,b+36,4);need(!data,"Native null material payload");}
            result.values.push_back(std::move(v));
        }
        for(int i=0;i<count;++i)native.cleanup(slots[size_t(i)+1]);need(*reinterpret_cast<uint32_t*>(native.image.base+0x10db14)==0,"Original replacements/output must release all pool objects");notifications+=native.notifications;
#else
        const auto oldEnvironment=materialTrigX87EnvironmentCandidate();const auto effectiveFlags=(q[0]&16)?materialParameterFlagNamesCandidate():flagNames;need(materialParameterCollectionCandidate(result,parameters,effectiveFlags,base,(q[0]&8)?&over:nullptr,env),"Owned collection refusal");need(materialTrigX87EnvironmentCandidate()==oldEnvironment,"Owned x87 control/stack changed");
#endif
#ifndef SFM64_NATIVE_UNLIT_CONCRETE
        std::vector<MaterialParameterCompletedValueCandidate>values;for(const auto&v:result.values)values.push_back({v,0});
        for(uint32_t repeat=0;repeat<repeats;++repeat){
            UnlitGenericCompletionCandidate completed;
            need(unlitGenericCompletionCandidate(completed,values,types,settings,hardwareLevel),"Owned concrete stage refusal");
            route=0;result.warnings+=completed.warnings;values=std::move(completed.completed.values);
        }
        result.values.clear();for(auto&v:values)result.values.push_back(std::move(v.value));
#endif
        write(out,result);out.write(reinterpret_cast<const char*>(&route),4);++cases;
    }
    out.flush();need(cases&&in.eof()&&in.gcount()==0&&bool(out),"Collection framing/output");_mm_setcsr(oldMxcsr);
#ifdef SFM64_NATIVE_UNLIT_CONCRETE
    native.finish();RemoveVectoredExceptionHandler(observer);_controlfp_s(&control,oldControl,_MCW_RC|_MCW_PC);
    std::cout<<"native_notifications="<<notifications<<" stage_calls="<<native.stageCalls<<" native_allocations="<<native.allocations<<" native_releases="<<native.releases<<'\n';
#endif
    std::cout<<"PASS unlit_concrete_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

#ifndef SFM64_NATIVE_UNLIT_CONCRETE
static int analytical(){
    unsigned checks=0,refusals=0;auto check=[&](bool b){need(b,"Unlit analytical check");++checks;};
    std::vector<std::string>names;for(auto n:unlitGenericParameterNamesCandidate())names.emplace_back(n);
    MaterialParameterCollectionCandidate c;check(materialParameterCollectionCandidate(c,names,materialParameterFlagNamesCandidate(),{},nullptr,{}));
    std::vector<MaterialParameterCompletedValueCandidate>input;for(const auto&v:c.values)input.push_back({v,0});const auto types=unlitGenericParameterTypesCandidate();
    UnlitGenericCompletionCandidate out;check(unlitGenericCompletionCandidate(out,input,types,{},92));
    check(out.completed.values[16].value.value.numeric.vector[0]==0x40800000&&out.completed.values[44].value.value.numeric.vector[0]==0x3f000000&&out.completed.values[9].value.value.text=="effects/flashlight001");
    auto snapshot=[](const auto&r){MaterialParameterCollectionCandidate c;for(const auto&v:r.completed.values)c.values.push_back(v.value);std::ostringstream s;write(s,c);s<<r.warnings;return s.str();};const auto before=snapshot(out);
    auto wrong=types;wrong[6]=7;check(!unlitGenericCompletionCandidate(out,input,wrong,{},92)&&snapshot(out)==before);++refusals;
    auto missing=input;missing.resize(67);check(!unlitGenericCompletionCandidate(out,missing,types,{},92)&&snapshot(out)==before);++refusals;
    auto broken=input;broken[6].value.created=false;check(!unlitGenericCompletionCandidate(out,broken,types,{},92)&&snapshot(out)==before);++refusals;
    auto excessive=input;excessive.resize(257,input[0]);check(!unlitGenericCompletionCandidate(out,excessive,types,{},92)&&snapshot(out)==before);++refusals;
    auto extra=input;auto field=input[0];field.value.name="$decalscale";check(materialParameterCompletionSetCandidate(field,1,{0x3d4ccccd,0,0,0}));extra.push_back(field);check(unlitGenericCompletionCandidate(out,extra,types,{},92)&&out.completed.values.size()==69&&out.completed.values[68].value.name=="$decalscale"&&out.completed.values[68].value.value.numeric.vector[0]==0x3d4ccccd);
    check((input[16].value.value.numeric.typeAndMetadata&15)==6);
    std::cout<<"PASS analytical_checks="<<checks<<" refusals="<<refusals<<'\n';return 0;
}
#endif
