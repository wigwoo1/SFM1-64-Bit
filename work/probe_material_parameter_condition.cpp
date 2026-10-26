#include "material_parameter_condition_candidate.h"
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef SFM64_NATIVE_PARAMETER_CONDITION
#define NOMINMAX
#include <windows.h>
#include "material_parameter_condition_native_fixture.h"
static size_t activeConditionCase=0;
static LONG CALLBACK conditionException(EXCEPTION_POINTERS*p){
    std::fprintf(stderr,"condition_case=%zu code=%08lx eip=%08lx address=%08lx\n",activeConditionCase,p->ExceptionRecord->ExceptionCode,p->ContextRecord->Eip,p->ExceptionRecord->NumberParameters>1?static_cast<unsigned long>(p->ExceptionRecord->ExceptionInformation[1]):0ul);
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static int analytical(){
    unsigned checks=0,refusals=0;auto check=[&](bool b){need(b,"Condition analytical check");++checks;};
    MaterialParameterConditionCandidate value{true,true,true};MaterialParameterConditionEnvironmentCandidate state;
    check(materialParameterConditionCandidate(value,"360?$color2",state)&&value.skip&&value.conditional&&!value.unrecognized);
    check(materialParameterConditionCandidate(value,"!360?$color2",state)&&!value.skip&&value.conditional&&!value.unrecognized);
    check(materialParameterConditionCandidate(value,"unknown?$x",state)&&value.skip&&value.unrecognized);
    check(materialParameterConditionCandidate(value,"!unknown?$x",state)&&!value.skip&&value.unrecognized);
    check(materialParameterConditionCandidate(value,"?$x",state)&&!value.skip&&!value.conditional&&!value.unrecognized);
    state.srgbReturn=0x100;check(materialParameterConditionCandidate(value,"srgb?$x",state)&&value.skip);
    state.srgbReturn=0x80;check(materialParameterConditionCandidate(value,"SRGB?$x",state)&&!value.skip);
    state.hdr=-1;check(materialParameterConditionCandidate(value,"HDR?$x",state)&&!value.skip);
    check(materialParameterConditionCandidate(value,"hdr ?$x",state)&&value.unrecognized&&value.skip);
    const auto before=value;
    for(const auto&name:std::vector<std::string>{std::string(1024,'x'),std::string(256,'x')+"?$x"}){
        check(!materialParameterConditionCandidate(value,name,state)&&value.skip==before.skip&&value.conditional==before.conditional&&value.unrecognized==before.unrecognized);++refusals;
    }
    std::cout<<"PASS analytical_checks="<<checks<<" refusals="<<refusals<<'\n';return 0;
}
int main(int argc,char**argv){try{
    if(argc==2&&std::string(argv[1])=="--test")return analytical();
    need(argc==4&&!std::ifstream(argv[3],std::ios::binary).good(),"Library query fresh-output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"Condition IO");
#ifdef SFM64_NATIVE_PARAMETER_CONDITION
    static_assert(sizeof(void*)==4,"Private reference x86");MaterialParameterConditionNativeFixture native(argv[1]);
    auto observer=AddVectoredExceptionHandler(1,&conditionException);need(observer!=nullptr,"Native exception observer");
    unsigned hdrCalls=0,srgbCalls=0,ownerCalls=0;
#else
    static_assert(sizeof(void*)==8,"Owned x64");
#endif
    std::array<uint32_t,6>query{};size_t count=0;
    while(in.read(reinterpret_cast<char*>(query.data()),sizeof(query))){
        need(query[4]<=255&&query[5]<=1023,"Condition query domain");
        std::string name(query[5],'\0');need(bool(in.read(name.data(),name.size())),"Condition query name");const auto before=name;
        std::array<uint32_t,3>result{};
#ifdef SFM64_NATIVE_PARAMETER_CONDITION
        const auto question=std::strchr(name.c_str(),'?');need(!question||size_t(question-name.c_str())<=255,"Unsafe native prefix excluded");
        activeConditionCase=count;native.inputName=name;native.prepare(query[0],query[1],query[2],query[3]);
        std::array<uint8_t,32>node{};std::array<uint8_t,3>flag{0xb6,uint8_t(query[4]),0xc7};
        using Condition=bool(__thiscall*)(void*,void*,uint8_t*);
        result[0]=reinterpret_cast<Condition>(0x100055d0)(&native.owner,node.data(),flag.data()+1)?1u:0u;
        result[1]=flag[1];result[2]=native.warnings;
        need(flag[0]==0xb6&&flag[2]==0xc7&&flag[1]<=1&&native.nameLookups==1,"Native flag guard/name access");
        need(native.ownerNameCalls==native.warnings&&native.warnings<=1&&native.hdrCalls+native.srgbCalls<=1,"Native condition services");
        hdrCalls+=native.hdrCalls;srgbCalls+=native.srgbCalls;ownerCalls+=native.ownerNameCalls;
#else
        MaterialParameterConditionEnvironmentCandidate state;std::memcpy(&state.lowFill,&query[0],4);std::memcpy(&state.hdr,&query[1],4);state.srgbReturn=query[2];std::memcpy(&state.gpu,&query[3],4);
        MaterialParameterConditionCandidate value{true,true,true};need(materialParameterConditionCandidate(value,name,state),"Owned name refusal");
        result={value.skip?1u:0u,value.conditional?1u:0u,value.unrecognized?1u:0u};
#endif
        need(name==before,"Condition input changed");out.write(reinterpret_cast<const char*>(result.data()),sizeof(result));++count;
    }
    out.flush();need(count&&in.eof()&&in.gcount()==0&&bool(out),"Condition framing/output");
#ifdef SFM64_NATIVE_PARAMETER_CONDITION
    native.image.verify();need(!native.allocations&&!native.releases&&!native.queues,"Unexpected native allocation/queue");
    RemoveVectoredExceptionHandler(observer);
    std::cout<<"native_hdr_calls="<<hdrCalls<<" native_srgb_calls="<<srgbCalls<<" native_owner_warning_calls="<<ownerCalls<<'\n';
#endif
    std::cout<<"PASS condition_cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
