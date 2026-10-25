#include "material_matrix_math_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#ifdef SFM64_NATIVE_MATRIX_MATH
#define NOMINMAX
#include <windows.h>
#include <float.h>
#include "material_construction_native_fixture.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int main(int argc,char**argv){try{
    need(argc==4&&!std::ifstream(argv[3],std::ios::binary).good(),"library queries fresh-output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"Matrix math IO");
#ifdef SFM64_NATIVE_MATRIX_MATH
    static_assert(sizeof(void*)==4,"Reference x86");MaterialConstructionNativeFixture f(argv[1]);
    using Init=void(__cdecl*)();reinterpret_cast<Init>(0x1007d4e7)();
    unsigned old=0,control=0;_controlfp_s(&old,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
#else
    static_assert(sizeof(void*)==8,"Owned x64");
#endif
    const auto oldMxcsr=_mm_getcsr();_mm_setcsr(0x1f80);std::array<uint32_t,34>q{};size_t cases=0;
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
        need(q[0]<=4&&q[1]<=3,"Matrix math query");MaterialMatrixMathCandidate a{},b{},result{};
        std::copy(q.begin()+2,q.begin()+18,a.begin());std::copy(q.begin()+18,q.end(),b.begin());
        auto multiply=[&](auto&x,auto&y,auto&z){
#ifdef SFM64_NATIVE_MATRIX_MATH
            using Fn=void(__cdecl*)(const void*,const void*,void*);reinterpret_cast<Fn>(0x1006cb00)(x.data(),y.data(),z.data());
#else
            materialMatrixMultiplyCandidate(x,y,z);
#endif
        };
        if(q[0]==0){
            if(q[1]==0)multiply(a,b,result);else if(q[1]==1){multiply(a,b,a);result=a;}
            else if(q[1]==2){multiply(a,b,b);result=b;}else{multiply(a,a,a);result=a;}
        }else if(q[0]<=3){
#ifdef SFM64_NATIVE_MATRIX_MATH
            using Fn=void(__cdecl*)(void*,float,float,float);
            const uintptr_t address=q[0]==1?0x1006d040:q[0]==2?0x1006d240:0x1006d190;
            reinterpret_cast<Fn>(address)(result.data(),materialMatrixFloatCandidate(a[0]),materialMatrixFloatCandidate(a[1]),materialMatrixFloatCandidate(a[2]));
#else
            result=q[0]==1?materialMatrixTranslateCandidate(a[0],a[1],a[2]):q[0]==2?materialMatrixScaleMathCandidate(a[0],a[1],a[2]):materialMatrixRotateCandidate(a[0]);
#endif
        }else{
#ifdef SFM64_NATIVE_MATRIX_MATH
            using Fn=void(__cdecl*)(void*,float,float,float);auto translate=reinterpret_cast<Fn>(0x1006d040),scale=reinterpret_cast<Fn>(0x1006d240),rotate=reinterpret_cast<Fn>(0x1006d190);
            MaterialMatrixMathCandidate stage{};
            translate(result.data(),materialMatrixFloatCandidate(a[0]^0x80000000u),materialMatrixFloatCandidate(a[1]^0x80000000u),0);
            scale(stage.data(),materialMatrixFloatCandidate(a[2]),materialMatrixFloatCandidate(a[3]),1);multiply(stage,result,result);
            rotate(stage.data(),materialMatrixFloatCandidate(a[4]),0,0);multiply(stage,result,result);
            const float x=materialMatrixFloatCandidate(a[5])+materialMatrixFloatCandidate(a[0]),y=materialMatrixFloatCandidate(a[6])+materialMatrixFloatCandidate(a[1]);
            translate(stage.data(),x,y,0);multiply(stage,result,result);
#else
            std::array<uint32_t,7>p{};std::copy(a.begin(),a.begin()+7,p.begin());result=materialMatrixTransformCandidate(p);
#endif
        }
        out.write(reinterpret_cast<const char*>(result.data()),64);++cases;
    }
    out.flush();need(in.eof()&&in.gcount()==0&&cases&&bool(out),"Matrix math output framing");_mm_setcsr(oldMxcsr);
#ifdef SFM64_NATIVE_MATRIX_MATH
    f.image.verify();need(!f.allocations&&!f.queues,"Unexpected material services");_controlfp_s(&control,old,_MCW_RC|_MCW_PC);
#endif
    std::cout<<"PASS matrix_math_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
