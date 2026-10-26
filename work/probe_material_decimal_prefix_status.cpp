#include "material_decimal_prefix_candidate.h"
#include <cerrno>
#include <fstream>
#include <iostream>
#include <vector>
#include <xmmintrin.h>
#ifdef SFM64_NATIVE_PREFIX_STATUS
#define NOMINMAX
#include <windows.h>
#include <float.h>
#include "source_material_patch_policy_candidate.h"
#include "native_material_keyvalues_fixture.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t word(std::istream&in){uint32_t n=0;need(bool(in.read(reinterpret_cast<char*>(&n),4)),"Prefix input word");return n;}
int main(int argc,char**argv){try{
    need(argc==4&&!std::ifstream(argv[3],std::ios::binary).good(),"Library input fresh-output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"Prefix IO");
#ifdef SFM64_NATIVE_PREFIX_STATUS
    static_assert(sizeof(void*)==4,"Native reference x86");NativeMaterialKeyValuesFixture native(argv[1]);
    using Parse=double(__cdecl*)(const char*,const char**);using Errno=int*(__cdecl*)();
    const auto parse=reinterpret_cast<Parse>(native.at(0x7e4b0));
    const auto originalErrno=reinterpret_cast<Errno>(native.at(0x82f6d))();
    unsigned old=0,control=0;_controlfp_s(&old,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
#else
    static_assert(sizeof(void*)==8,"Owned x64");
#endif
    const auto oldMxcsr=_mm_getcsr();_mm_setcsr(0x1f80);const auto count=word(in);need(count&&count<=100000,"Prefix case count");
    for(uint32_t i=0;i<count;++i){
        const auto size=word(in);need(size<=32767,"Prefix input extent");std::string s(size,'\0');need(bool(in.read(s.data(),size)),"Prefix string");const auto before=s;
        double value=0;uint32_t consumed=0,range=0;
#ifdef SFM64_NATIVE_PREFIX_STATUS
        const char*end=nullptr;*originalErrno=0;value=parse(s.c_str(),&end);consumed=uint32_t(end-s.c_str());range=*originalErrno==ERANGE?1u:0u;
#else
        const int saved=errno;errno=EDOM;const auto parsed=materialDecimalPrefixCandidate(s);need(errno==EDOM,"Owned prefix changed caller errno");errno=saved;
        value=parsed.value;consumed=uint32_t(parsed.consumed);range=parsed.rangeError?1u:0u;
#endif
        need(s==before&&consumed<=s.size(),"Prefix input mutation or invalid end");
        out.write(reinterpret_cast<const char*>(&value),8);out.write(reinterpret_cast<const char*>(&consumed),4);out.write(reinterpret_cast<const char*>(&range),4);
    }
    need(in.peek()==std::char_traits<char>::eof(),"Prefix trailing input");out.flush();need(bool(out),"Prefix output");_mm_setcsr(oldMxcsr);
#ifdef SFM64_NATIVE_PREFIX_STATUS
    native.verify();_controlfp_s(&control,old,_MCW_RC|_MCW_PC);
#endif
    std::cout<<"PASS prefix value/end/range cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
