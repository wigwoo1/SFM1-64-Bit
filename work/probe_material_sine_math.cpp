#ifdef SFM64_NATIVE_SINE_MATH
#define SFM64_NATIVE_OBJECT_PARAMETERS
#endif
#define SFM64_OBJECT_PARAMETER_MAIN sineMathPriorParameterMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#include <cmath>
#ifndef SFM64_NATIVE_SINE_MATH
#include "material_sine_value_candidate.h"
#endif
#ifdef SFM64_NATIVE_SINE_MATH
#include "material_float_input_client_native_fixture.h"
#endif
// Experimental comparison only: do not register this as an owned Sine until
// the original clock/math behavior and the numeric implementation are proved.
#ifndef SFM64_NATIVE_SINE_MATH
static uint32_t experimentalSine(const std::array<uint32_t,5>&q){return materialSineValueCandidate(q);}
#endif
int main(int argc,char**argv){try{
    gate((argc==4||(argc==5&&std::string(argv[4])=="--sse2"))&&!std::ifstream(argv[3]).good(),"client-or-absent query fresh-output [--sse2]");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);gate(bool(in)&&bool(out),"Sine math IO");
#ifdef SFM64_NATIVE_SINE_MATH
    SetErrorMode(SEM_NOGPFAULTERRORBOX|SEM_FAILCRITICALERRORS);auto observer=AddVectoredExceptionHandler(1,&lifetimeFault);MaterialFloatInputClientNativeFixture client(argv[1]);std::array<uint32_t,16>clock{};client.word(0x63ce48,reinterpret_cast<uintptr_t>(clock.data()));unsigned old=0,control=0;_controlfp_s(&old,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
    MaterialObjectParameterStateCandidate seed;MaterialTypedDispatchCandidate output;output.name="$out";output.created=true;output.value.numeric.typeAndMetadata=0;seed.material.parameters.values.push_back(output);
    client.word(0x854fc4,argc==5?1:0); // Exercise both original fallback-dispatch states.
#else
    static_assert(sizeof(void*)==8,"Owned experimental math x64");
#endif
    const auto mx=_mm_getcsr();_mm_setcsr(0x1f80);unsigned count=0;std::array<uint32_t,5>q{};
    while(in.read(reinterpret_cast<char*>(q.data()),20)){
        uint32_t result=0;MaterialNumericValueCandidate value;value.typeAndMetadata=0;
#ifdef SFM64_NATIVE_SINE_MATH
        currentCase=count;client.prepare(seed);SourceMaterialPatchNodeCandidate declaration;declaration.key="Sine";declaration.children={{"resultVar",1,"$out"},{"sineperiod",3,"",q[1]},{"sinemin",3,"",q[2]},{"sinemax",3,"",q[3]},{"timeOffset",3,"",q[4]}};
        using Create=void*(__cdecl*)();auto*p=reinterpret_cast<Create>(0x10137370)();gate(p&&*static_cast<uintptr_t*>(p)==0x104fff24,"Actual Sine factory/vtable");using Init=bool(__thiscall*)(void*,void*,void*);gate(reinterpret_cast<Init>(0x10136440)(p,&client.material,client.build(declaration)),"Actual Sine Init");clock[3]=q[0];client.writes.clear();using Bind=void(__thiscall*)(void*,void*);reinterpret_cast<Bind>(0x101364f0)(p,nullptr);gate(client.writes.size()==1&&client.writes[0].operation==1,"Actual Sine scalar dispatch");result=client.writes[0].arguments[0];value=client.variables.front().value.value.numeric;using Release=void(__thiscall*)(void*);reinterpret_cast<Release>(0x10175ed0)(p);gate(client.blocks.empty(),"Sine per-case allocation balance");
#else
        result=experimentalSine(q);gate(materialNumericSetCandidate(value,MaterialNumericOpCandidate::SetFloat,{result,0,0,0}),"Experimental scalar output");
#endif
        put(out,result);out.write(reinterpret_cast<const char*>(&value),24);++count;
    }
    gate(in.eof()&&in.gcount()==0&&count>0,"Sine math framing");out.flush();gate(bool(out),"Sine complete output");_mm_setcsr(mx);
#ifdef SFM64_NATIVE_SINE_MATH
    client.image.verify();gate(client.allocations==client.releases,"Sine final allocation balance");_controlfp_s(&control,old,_MCW_RC|_MCW_PC);RemoveVectoredExceptionHandler(observer);std::cout<<"native_proxy_allocations="<<client.allocations<<" releases="<<client.releases<<'\n';
#endif
    std::cout<<"PASS sine_math_cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
