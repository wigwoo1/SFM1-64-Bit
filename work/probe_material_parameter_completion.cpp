#include "material_parameter_completion_candidate.h"
struct CompletionAction{uint32_t index=0,operation=0;std::array<uint32_t,4>bits{};};
#include "material_parameter_flags_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <cstring>
#include <sstream>
#ifdef SFM64_NATIVE_PARAMETER_COMPLETION
#define NOMINMAX
#include <windows.h>
#include <float.h>
#include "material_parameter_completion_native_fixture.h"
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
#ifndef SFM64_NATIVE_PARAMETER_COMPLETION
static int analytical(){
    unsigned checks=0,refusals=0;auto check=[&](bool b){if(!b)throw std::runtime_error("Completion analytical check="+std::to_string(checks));++checks;};
    MaterialParameterCollectionCandidate c;MaterialParameterCollectionEnvironmentCandidate env;
    const std::vector<std::string>names={"$flags","$flags_defined","$flags2","$flags_defined2","$color","$alpha","$x"};
    check(materialParameterCollectionCandidate(c,names,{},{{"$x","[1 2 3]",1,0}},nullptr,env));
    std::vector<MaterialParameterCompletedValueCandidate>input;for(const auto&v:c.values)input.push_back({v,0});
    MaterialParameterCompletionCandidate out;std::vector<uint32_t>types={1,1,1,1,0,0,10};
    check(materialParameterCompletionCandidate(out,input,types,[](auto&){return true;})&&(out.values[6].value.value.numeric.typeAndMetadata&255)==0x32);
    auto snapshot=[](const auto&r){MaterialParameterCollectionCandidate flat;for(const auto&v:r.values)flat.values.push_back(v.value);std::ostringstream s;write(s,flat);for(const auto&v:r.values)s<<v.nullResource;for(auto i:r.descriptorLookups)s<<i;return s.str();};
    const auto before=snapshot(out);auto same=[&](){return snapshot(out)==before;};
    auto bad=types;bad.resize(5);check(!materialParameterCompletionCandidate(out,input,bad,[](auto&){return true;})&&same());++refusals;
    check(!materialParameterCompletionCandidate(out,input,types,[](auto&v){v[4].value.value.numeric.integer=7;return false;})&&same());++refusals;
    check(!materialParameterCompletionCandidate(out,input,types,[](auto&v){v.pop_back();return true;})&&same());++refusals;
    check(!materialParameterCompletionCandidate(out,input,types,[](auto&v){v[0].value.name="changed";return true;})&&same());++refusals;
    auto invalid=input;invalid[6].value.value.numeric.typeAndMetadata=3;
    check(!materialParameterCompletionCandidate(out,invalid,types,[](auto&){return true;})&&same());++refusals;
    check(materialParameterCompletionCandidate(out,input,types,[](auto&v){return materialParameterCompletionSetCandidate(v[6],6);})&&out.values[6].value.matrixIdentity);
    check((input[6].value.value.numeric.typeAndMetadata&255)==0x32&&!input[6].value.matrix);
    auto v=out.values[6];const auto copy=v;check(!materialParameterCompletionSetCandidate(v,5)&&v.value.matrix==copy.value.matrix&&v.value.value.numeric.typeAndMetadata==copy.value.value.numeric.typeAndMetadata);++refusals;
    std::cout<<"PASS analytical_checks="<<checks<<" refusals="<<refusals<<'\n';return 0;
}
#endif
int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_PARAMETER_COMPLETION
    if(argc==2&&std::string(argv[1])=="--test")return analytical();
#endif
    need(argc==4&&!std::ifstream(argv[3],std::ios::binary).good(),"Library query fresh-output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"Collection IO");
#ifdef SFM64_NATIVE_PARAMETER_COMPLETION
    static_assert(sizeof(void*)==4,"Private reference x86");MaterialParameterCompletionNativeFixture native(argv[1]);auto observer=AddVectoredExceptionHandler(1,&exceptionObserver);need(observer!=nullptr,"Exception observer");
    unsigned oldControl=0,control=0;_controlfp_s(&oldControl,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);size_t notifications=0;
#else
    static_assert(sizeof(void*)==8,"Owned x64");
#endif
    const auto oldMxcsr=_mm_getcsr();_mm_setcsr(0x1f80);size_t cases=0;std::array<uint32_t,12>q{};
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
        need(q[0]<=31&&q[8]>=6&&q[8]<=256&&q[9]<=32&&q[10]+q[11]<=512,"Collection input counts");
        // Conservative native bound: even if every supplied node appends an
        // extra, the original unchecked 256-entry output table cannot overflow.
        need(q[8]+q[10]+q[11]<=256,"Safe native output domain");
        MaterialParameterCollectionEnvironmentCandidate env;env.model=(q[0]&1)!=0;env.numericSse2=(q[0]&2)!=0;env.trigSse2=(q[0]&4)!=0;
        std::memcpy(&env.condition.lowFill,&q[1],4);std::memcpy(&env.condition.hdr,&q[2],4);env.condition.srgbReturn=q[3];std::memcpy(&env.condition.gpu,&q[4],4);env.percentGateReturn=q[5];env.percentKeepReturn=q[6];std::memcpy(&env.percentOverride,&q[7],4);
        std::vector<std::string>parameters,flagNames;for(uint32_t i=0;i<q[8];++i)parameters.push_back(text(in,1023));for(uint32_t i=0;i<q[9];++i)flagNames.push_back(text(in,1023));
        std::vector<MaterialParameterInputCandidate>base,over;
        for(uint32_t i=0;i<q[10]+q[11];++i){MaterialParameterInputCandidate n;n.type=word(in);n.bits=word(in);n.name=text(in,1023);n.text=text(in,4095);need(n.type<=255&&n.type!=5,"Native type domain");const auto pos=n.name.find('?');need(pos==std::string::npos||pos<=255,"Native prefix domain");(i<q[10]?base:over).push_back(std::move(n));}
        const auto repeats=word(in),actionCount=word(in);need(repeats>=1&&repeats<=3&&actionCount<=256,"Stage counts");
        std::vector<uint32_t>types;for(uint32_t i=0;i<q[8];++i)types.push_back(word(in));
        std::vector<CompletionAction>actions;for(uint32_t i=0;i<actionCount;++i){CompletionAction a;a.index=word(in);a.operation=word(in);for(auto&v:a.bits)v=word(in);need(a.index<q[8]&&(a.operation<=4||a.operation==6),"Stage action");actions.push_back(a);}
        std::vector<uint32_t>lookups;MaterialParameterCollectionCandidate result;
#ifdef SFM64_NATIVE_PARAMETER_COMPLETION
        activeCase=cases;native.parameters=parameters;native.flags=flagNames;native.prepare(env,base,over,(q[0]&16)!=0);
        std::array<uint8_t,32>baseRoot{},overRoot{};const uintptr_t baseChild=base.empty()?0:reinterpret_cast<uintptr_t>(native.nodes[0].data()),overChild=over.empty()?0:reinterpret_cast<uintptr_t>(native.nodes[base.size()].data());std::memcpy(baseRoot.data()+24,&baseChild,4);std::memcpy(overRoot.data()+24,&overChild,4);
        std::array<void*,258>slots{};slots.front()=reinterpret_cast<void*>(0x11223344);slots.back()=reinterpret_cast<void*>(0x55667788);
        using Collect=int(__thiscall*)(void*,void*,void*,void*,bool,void**);
        const int count=reinterpret_cast<Collect>(0x10005940)(&native.owner,&native.shader,baseRoot.data(),(q[0]&8)?overRoot.data():nullptr,env.model,slots.data()+1);
        need(count>=4&&count<=256&&slots.front()==reinterpret_cast<void*>(0x11223344)&&slots.back()==reinterpret_cast<void*>(0x55667788),"Native result bounds/guards");
        result.warnings=native.warnings;
        for(uint32_t repeat=0;repeat<repeats;++repeat){native.complete(slots.data()+1,types,actions);lookups.insert(lookups.end(),native.lookups.begin(),native.lookups.end());}
        need(result.warnings==native.warnings,"Unexpected diagnostic outside empty-log domain");
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
#ifndef SFM64_NATIVE_PARAMETER_COMPLETION
        std::vector<MaterialParameterCompletedValueCandidate>values;for(const auto&v:result.values)values.push_back({v,0});
        for(uint32_t repeat=0;repeat<repeats;++repeat){
            MaterialParameterCompletionCandidate completed;
            need(materialParameterCompletionCandidate(completed,values,types,[&](auto&v){for(const auto&a:actions)if(!materialParameterCompletionSetCandidate(v[a.index],a.operation,a.bits))return false;return true;}),"Owned stage refusal");
            lookups.insert(lookups.end(),completed.descriptorLookups.begin(),completed.descriptorLookups.end());values=std::move(completed.values);
        }
        result.values.clear();for(auto&v:values)result.values.push_back(std::move(v.value));
#endif
        write(out,result);const uint32_t lookupCount=uint32_t(lookups.size());out.write(reinterpret_cast<const char*>(&lookupCount),4);out.write(reinterpret_cast<const char*>(lookups.data()),4*lookups.size());++cases;
    }
    out.flush();need(cases&&in.eof()&&in.gcount()==0&&bool(out),"Collection framing/output");_mm_setcsr(oldMxcsr);
#ifdef SFM64_NATIVE_PARAMETER_COMPLETION
    native.finish();RemoveVectoredExceptionHandler(observer);_controlfp_s(&control,oldControl,_MCW_RC|_MCW_PC);
    std::cout<<"native_notifications="<<notifications<<" stage_calls="<<native.stageCalls<<" native_allocations="<<native.allocations<<" native_releases="<<native.releases<<'\n';
#endif
    std::cout<<"PASS completion_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
