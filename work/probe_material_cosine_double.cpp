#ifdef SFM64_NATIVE_COSINE_DOUBLE
#define SFM64_NATIVE_OBJECT_PARAMETERS
#endif
#define SFM64_OBJECT_PARAMETER_MAIN cosinePriorParameterMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#ifdef SFM64_NATIVE_COSINE_DOUBLE
#include "material_float_input_client_native_fixture.h"
#else
#include "material_cosine_double_candidate.h"
#endif
int main(int argc,char**argv){try{
    gate((argc==4||(argc==5&&std::string(argv[4])=="--sse2"))&&!std::ifstream(argv[3]).good(),"client-or-absent query fresh-output [--sse2]");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);gate(bool(in)&&bool(out),"Cosine IO");
#ifdef SFM64_NATIVE_COSINE_DOUBLE
    SetErrorMode(SEM_NOGPFAULTERRORBOX|SEM_FAILCRITICALERRORS);auto observer=AddVectoredExceptionHandler(1,&lifetimeFault);MaterialFloatInputClientNativeFixture f(argv[1]);f.word(0x854fc4,argc==5?1:0);unsigned cw=0,control=0;_controlfp_s(&cw,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
#else
    static_assert(sizeof(void*)==8,"Independent double cosine x64");
#endif
    auto mx=_mm_getcsr();_mm_setcsr(0x1f80);uint64_t q=0;unsigned cases=0;
    while(in.read(reinterpret_cast<char*>(&q),8)){const auto before=q;double value;std::memcpy(&value,&q,8);double result;
#ifdef SFM64_NATIVE_COSINE_DOUBLE
        using Cosine=double(__vectorcall*)(double);result=reinterpret_cast<Cosine>(0x1045cc7b)(value);f.image.verify();
#else
        result=materialCosineDoubleCandidate(value);
#endif
        gate(q==before,"Query unchanged");out.write(reinterpret_cast<const char*>(&result),8);++cases;
    }
    gate(in.eof()&&in.gcount()==0&&cases,"Complete cosine query");out.flush();gate(bool(out),"Complete cosine output");_mm_setcsr(mx);
#ifdef SFM64_NATIVE_COSINE_DOUBLE
    _controlfp_s(&control,cw,_MCW_RC|_MCW_PC);RemoveVectoredExceptionHandler(observer);
#endif
    std::cout<<"PASS cosine_double_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
