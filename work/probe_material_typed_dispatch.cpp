#include "material_typed_dispatch_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#include <map>
#ifdef SFM64_NATIVE_TYPED_DISPATCH
#define NOMINMAX
#include <windows.h>
#include <float.h>
#include "material_typed_dispatch_native_fixture.h"
static size_t activeCase=0;
static LONG CALLBACK observeException(EXCEPTION_POINTERS*p){
    std::fprintf(stderr,"private_case=%zu code=%08lx eip=%08lx address=%08lx\n",activeCase,p->ExceptionRecord->ExceptionCode,p->ContextRecord->Eip,p->ExceptionRecord->NumberParameters>1?static_cast<unsigned long>(p->ExceptionRecord->ExceptionInformation[1]):0ul);
    return EXCEPTION_CONTINUE_SEARCH;
}
static __declspec(noinline) void*nativeDispatch(void*node){
    void*result=nullptr;const uintptr_t address=0x100032a0;
    __asm { mov eax,node }
    __asm { xor ecx,ecx }
    __asm { call address }
    __asm { mov result,eax }
    return result;
}
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
#ifndef SFM64_NATIVE_TYPED_DISPATCH
static void analytical(){
    unsigned checks=0,refusals=0;MaterialTypedDispatchCandidate value;
    auto check=[&](bool b){need(b,"Typed dispatch analytical check");++checks;};
    auto make=[&](uint32_t type,uint32_t bits,std::string_view name,std::string_view text,bool pool=true){return materialTypedDispatchCandidate(value,type,bits,name,text,true,true,pool);};
    const std::string identity="[1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1";
    check(make(1,0,"A?B?C",identity)&&value.created&&value.matrix&&value.matrixIdentity&&value.name=="b?c");
    check(make(1,0,"x","[1 2 3]")&&value.created&&!value.matrix&&(value.value.numeric.typeAndMetadata&0x7f)==0x32&&value.value.numeric.vector[3]==0);
    check(make(1,0,"x","{255 0 127.5}")&&value.created&&value.value.numeric.vector[0]==0x3f800000&&value.value.numeric.vector[2]==0x3f000000);
    check(make(1,0,"x","[x]")&&!value.created&&value.warnings==1);
    check(make(1,0,"x","1e3")&&value.created&&(value.value.numeric.typeAndMetadata&15)==1&&value.value.numeric.integer==1000);
    check(make(2,1000,"x","")&&value.created&&(value.value.numeric.typeAndMetadata&15)==4&&value.value.numeric.vector[0]==0x447a0000);
    check(make(1,0,"x","")&&!value.created);
    check(make(1,0,"x",identity,false)&&!value.created&&!value.warnings);
    check(make(1,0,"x",identity)&&value.created);const auto old=value;
    for(unsigned bad=0;bad<3;++bad){
        check(!materialTypedDispatchCandidate(value,bad==0?256:1,0,bad==1?std::string(1024,'x'):"x",bad==2?std::string(4096,'1'):"x",false,false));
        check(value.created==old.created&&value.matrixIdentity==old.matrixIdentity&&value.warnings==old.warnings&&value.name==old.name&&value.matrix==old.matrix&&value.value.text==old.value.text&&!std::memcmp(&value.value.numeric,&old.value.numeric,24));++refusals;
    }
    std::cout<<"PASS analytical_checks="<<checks<<" refusals="<<refusals<<'\n';
}
#endif
int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_TYPED_DISPATCH
    if(argc==2&&std::string(argv[1])=="--test"){analytical();return 0;}
#endif
    need(argc==4&&!std::ifstream(argv[3],std::ios::binary).good(),"library query fresh-output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"Dispatch IO");
#ifdef SFM64_NATIVE_TYPED_DISPATCH
    static_assert(sizeof(void*)==4,"Reference x86");MaterialTypedDispatchNativeFixture fixture(argv[1]);
    unsigned oldControl=0,control=0;_controlfp_s(&oldControl,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
    auto observer=AddVectoredExceptionHandler(1,&observeException);need(observer!=nullptr,"Exception observer");
    std::map<std::string,uint16_t>symbols;size_t reusedSymbols=0;
#else
    static_assert(sizeof(void*)==8,"Owned x64");
#endif
    const auto oldMxcsr=_mm_getcsr();_mm_setcsr(0x1f80);size_t cases=0;std::array<uint32_t,7>q{};
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
        need(q[0]<=255&&q[1]<=3&&q[2]<=1&&q[3]<=255&&q[5]<=1023&&q[6]<=4095,"Dispatch query domain");
        std::string name(q[5],'\0'),text(q[6],'\0');need(bool(in.read(name.data(),name.size()))&&bool(in.read(text.data(),text.size())),"Dispatch query framing");
        std::array<uint32_t,28>r{};std::string actualName,actualText;
#ifdef SFM64_NATIVE_TYPED_DISPATCH
        activeCase=cases;fixture.inputName=name;fixture.prepare((q[1]&1)!=0,(q[1]&2)!=0,q[2]!=0,uint8_t(q[3]));
        std::array<uint8_t,32>node{};const auto pointer=reinterpret_cast<uintptr_t>(text.c_str());
        std::memcpy(node.data()+4,&pointer,4);std::memcpy(node.data()+12,&q[4],4);node[16]=uint8_t(q[0]);
        void*p=nativeDispatch(node.data());r[0]=p?1u:0u;r[7]=fixture.warnings;
        if(p){
            need(p==fixture.object.data(),"Original factory result must be supplied pool slot");auto*b=static_cast<uint8_t*>(p);
            uintptr_t table=0,owner=0;std::memcpy(&table,b,4);std::memcpy(&owner,b+32,4);need(table==0x10098f34&&!owner,"Original constructed object");
            std::memcpy(&r[1],b+28,4);r[1]&=0xffffu;std::memcpy(&r[2],b+8,4);std::memcpy(r.data()+3,b+12,16);
            using GetName=const char*(__thiscall*)(void*);actualName=reinterpret_cast<GetName>(0x10017b80)(p);
            uint16_t symbol=0;std::memcpy(&symbol,b+30,2);need(symbol!=0xffffu,"Constructed name must be interned");
            const auto inserted=symbols.emplace(actualName,symbol);
            if(!inserted.second){need(inserted.first->second==symbol,"Original name reuse must retain symbol");++reusedSymbols;}
            uintptr_t data=0;
            if((r[1]&15)==1){std::memcpy(&data,b+4,4);actualText=reinterpret_cast<const char*>(data);}
            if((r[1]&15)==7){
                std::memcpy(&data,b+36,4);need(data!=0,"Original matrix storage");r[8]=1;
                std::memcpy(r.data()+12,reinterpret_cast<void*>(data),64);r[9]=*reinterpret_cast<uint8_t*>(data+64);
            }
            fixture.destroy(p);
        }
        need(!fixture.lockDepth,"Balanced constructor name locks");
#else
        const auto before=materialTrigX87EnvironmentCandidate();MaterialTypedDispatchCandidate value;
        need(materialTypedDispatchCandidate(value,q[0],q[4],name,text,(q[1]&1)!=0,(q[1]&2)!=0,q[2]!=0),"Owned dispatch input refusal");
        r[0]=value.created?1u:0u;r[7]=value.warnings;
        if(value.created){
            r[1]=value.value.numeric.typeAndMetadata;r[2]=value.value.numeric.integer;
            std::copy(value.value.numeric.vector.begin(),value.value.numeric.vector.end(),r.begin()+3);
            actualText=value.value.text;actualName=value.name;
            if(value.matrix){r[8]=1;r[9]=value.matrixIdentity?1u:0u;std::copy(value.matrix->begin(),value.matrix->end(),r.begin()+12);}
        }
        need(materialTrigX87EnvironmentCandidate()==before,"Owned x87 environment changed");
#endif
        r[10]=uint32_t(actualText.size());r[11]=uint32_t(actualName.size());out.write(reinterpret_cast<const char*>(r.data()),sizeof(r));out.write(actualText.data(),actualText.size());out.write(actualName.data(),actualName.size());++cases;
    }
    out.flush();need(in.eof()&&in.gcount()==0&&cases&&bool(out),"Dispatch output framing");_mm_setcsr(oldMxcsr);
#ifdef SFM64_NATIVE_TYPED_DISPATCH
    fixture.finish();RemoveVectoredExceptionHandler(observer);_controlfp_s(&control,oldControl,_MCW_RC|_MCW_PC);
    std::cout<<"native_allocations="<<fixture.allocations<<" native_releases="<<fixture.releases<<" unique_native_symbols="<<symbols.size()<<" reused_native_symbols="<<reusedSymbols<<'\n';
#endif
    std::cout<<"PASS typed_dispatch_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
