#ifdef SFM64_NATIVE_SCROLL_MATH
#define SFM64_NATIVE_OBJECT_PARAMETERS
#endif
#define SFM64_OBJECT_PARAMETER_MAIN scrollMathPriorParameterMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#ifdef SFM64_NATIVE_SCROLL_MATH
#include "material_sine_client_native_fixture.h"
struct ScrollMathNativeFixture:MaterialSineClientNativeFixture {
    using MaterialSineClientNativeFixture::MaterialSineClientNativeFixture;
    std::array<uint32_t,17>result{};unsigned sets=0;
    static ScrollMathNativeFixture&typed(){return *static_cast<ScrollMathNativeFixture*>(active);}
    static void __fastcall vector3(void*p,void*,uint32_t x,uint32_t y,uint32_t z){auto&f=typed();gate(f.index(p)==0,"Scroll result identity");f.result={3,x,y,z};++f.sets;}
    static void __fastcall matrix(void*p,void*,const uint32_t*m){auto&f=typed();gate(f.index(p)==0,"Scroll matrix identity");f.result[0]=9;std::copy_n(m,16,f.result.begin()+1);++f.sets;}
    void configure(){varMethods[10]=reinterpret_cast<uintptr_t>(&vector3);varMethods[19]=reinterpret_cast<uintptr_t>(&matrix);}
};
#else
#include "material_texture_scroll_value_candidate.h"
#endif
int main(int argc,char**argv){try{
    gate((argc==4||(argc==5&&std::string(argv[4])=="--sse2"))&&!std::ifstream(argv[3]).good(),"private-client-or-absent query fresh-output [--sse2]");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);gate(bool(in)&&bool(out),"Scroll math IO");
#ifdef SFM64_NATIVE_SCROLL_MATH
    SetErrorMode(SEM_NOGPFAULTERRORBOX|SEM_FAILCRITICALERRORS);auto observer=AddVectoredExceptionHandler(1,&lifetimeFault);ScrollMathNativeFixture f(argv[1]);f.configure();f.word(0x854fc4,argc==5?1:0);unsigned cw=0,control=0;_controlfp_s(&cw,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
#else
    static_assert(sizeof(void*)==8,"Independent scroll math x64");
#endif
    auto mx=_mm_getcsr();_mm_setcsr(0x1f80);std::array<uint32_t,5>q{};unsigned cases=0;
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){const auto before=q;std::array<uint32_t,17>result{};
#ifdef SFM64_NATIVE_SCROLL_MATH
        MaterialObjectParameterStateCandidate seed;MaterialTypedDispatchCandidate v;v.created=true;v.name="$out";v.value.numeric.typeAndMetadata=q[4];seed.material.parameters.values.push_back(v);f.prepare(seed);f.result={};f.sets=0;
        SourceMaterialPatchNodeCandidate d;d.key="TextureScroll";d.children={{"textureScrollVar",1,"$out"},{"textureScrollRate",3,"",q[1]},{"textureScrollAngle",3,"",q[2]},{"textureScale",3,"",q[3]}};
        using Create=void*(__cdecl*)();auto*p=reinterpret_cast<Create>(0x10198b60)();gate(p&&*static_cast<uintptr_t*>(p)==0x1050d454,"Actual TextureScroll factory");gate(f.initializeFunction(p,d)&&f.materialIdentity(p),"Actual TextureScroll Init/material");f.evaluateFunction(p,q[0]);gate(f.sets==1,"One complete scroll result");result=f.result;f.destroy(p);gate(f.blocks.empty(),"Actual scroll cleanup");f.image.verify();
#else
        result=materialTextureScrollValueCandidate(q);
#endif
        gate(q==before,"Input unchanged");out.write(reinterpret_cast<const char*>(result.data()),sizeof(result));++cases;
    }
    gate(in.eof()&&in.gcount()==0&&cases,"Complete query framing");out.flush();gate(bool(out),"Complete output");_mm_setcsr(mx);
#ifdef SFM64_NATIVE_SCROLL_MATH
    gate(f.allocations==f.releases,"Original proxy allocation balance");_controlfp_s(&control,cw,_MCW_RC|_MCW_PC);RemoveVectoredExceptionHandler(observer);std::cout<<"native_proxy_allocations="<<f.allocations<<" releases="<<f.releases<<'\n';
#endif
    std::cout<<"PASS texture_scroll_math_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
