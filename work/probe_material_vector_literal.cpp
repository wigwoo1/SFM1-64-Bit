#include "material_vector_literal_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#include <xmmintrin.h>
#ifdef SFM64_NATIVE_MATERIAL_VECTOR
#define NOMINMAX
#include <windows.h>
#include <float.h>
#include "material_construction_native_fixture.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int main(int argc,char**argv){try{
    need(argc==4&&!std::ifstream(argv[3],std::ios::binary).good(),"library queries fresh-output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"Vector IO");
#ifdef SFM64_NATIVE_MATERIAL_VECTOR
    static_assert(sizeof(void*)==4,"Reference x86");MaterialConstructionNativeFixture f(argv[1],true);
    unsigned old=0,control=0;_controlfp_s(&old,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
#else
    static_assert(sizeof(void*)==8,"Owned x64");
#endif
    const auto oldMxcsr=_mm_getcsr();_mm_setcsr(0x1f80);size_t cases=0;std::array<uint32_t,6>q{};
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
        need(q[0]<=1&&q[5]<=4095,"Query domain");std::string s(q[5],'\0');need(bool(in.read(s.data(),s.size())),"Text framing");
        std::array<uint32_t,8>r{};
        if(q[0]==0){double value=0;uint32_t consumed=0;
#ifdef SFM64_NATIVE_MATERIAL_VECTOR
            const char*end=nullptr;using Parse=double(__cdecl*)(const char*,const char**);
            value=reinterpret_cast<Parse>(0x1007e4b0)(s.c_str(),&end);consumed=uint32_t(end-s.c_str());
#else
            const auto parsed=materialDecimalPrefixCandidate(s);value=parsed.value;consumed=uint32_t(parsed.consumed);
#endif
            std::memcpy(r.data(),&value,8);r[2]=consumed;
        }else{
            MaterialVectorLiteralCandidate v;std::copy(q.begin()+1,q.begin()+5,v.words.begin());bool start=false;
#ifdef SFM64_NATIVE_MATERIAL_VECTOR
            using Start=bool(__thiscall*)(const char*);start=reinterpret_cast<Start>(0x10002980)(s.c_str());
            f.warnings=f.nameLookups=0;
            if(start){
                std::array<uint8_t,32>node{};const uintptr_t text=reinterpret_cast<uintptr_t>(s.c_str());std::memcpy(node.data()+4,&text,4);node[16]=1;
                using Parse=unsigned(__cdecl*)(const void*,const char*,void*);
                v.count=reinterpret_cast<Parse>(0x10002ec0)(node.data(),"$fixture",v.words.data());
                v.warnings=f.warnings;need(f.nameLookups==v.warnings,"Warning name calls");
            }
#else
            start=materialVectorLiteralStartCandidate(s);if(start)need(materialVectorLiteralCandidate(s,v),"Owned vector refusal");
#endif
            r[0]=start;r[1]=v.count;r[2]=v.warnings;std::copy(v.words.begin(),v.words.end(),r.begin()+3);
        }
        out.write(reinterpret_cast<const char*>(r.data()),sizeof(r));++cases;
    }
    out.flush();need(in.eof()&&in.gcount()==0&&cases&&bool(out),"Query/output framing");_mm_setcsr(oldMxcsr);
#ifdef SFM64_NATIVE_MATERIAL_VECTOR
    f.image.verify();need(!f.allocations&&!f.releases&&!f.queues,"Unexpected material services");_controlfp_s(&control,old,_MCW_RC|_MCW_PC);
#endif
    std::cout<<"PASS vector_prefix_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
