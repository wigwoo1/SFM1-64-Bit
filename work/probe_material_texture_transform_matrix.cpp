#ifdef SFM64_NATIVE_TEXTURE_TRANSFORM
#define SFM64_NATIVE_OBJECT_PARAMETERS
#endif
#define SFM64_OBJECT_PARAMETER_MAIN textureTransformPriorParameterMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#include "material_texture_transform_matrix_candidate.h"
#ifdef SFM64_NATIVE_TEXTURE_TRANSFORM
#include "material_float_input_client_native_fixture.h"
struct TextureTransformNativeFixture:MaterialFloatInputClientNativeFixture {
    MaterialMatrixMathCandidate result{};unsigned sets=0,gets=0;
    using MaterialFloatInputClientNativeFixture::MaterialFloatInputClientNativeFixture;
    static TextureTransformNativeFixture&typed(){return *static_cast<TextureTransformNativeFixture*>(active);}
    static void __fastcall getTwo(void*p,void*,uint32_t*out,int count){auto&f=typed();gate(count==2,"Exactly two cached input reads");const auto&v=f.variables[f.index(p)].value.value.numeric.vector;for(int i=0;i<count;++i)out[i]=materialTextureTransformReadCandidate(v[size_t(i)]);++f.gets;}
    static void __fastcall matrix(void*,void*,const uint32_t*p){auto&f=typed();std::copy_n(p,16,f.result.begin());++f.sets;}
    void configure(){varMethods[19]=reinterpret_cast<uintptr_t>(&matrix);varMethods[28]=reinterpret_cast<uintptr_t>(&getTwo);}
};
#endif
int main(int argc,char**argv){try{
    gate((argc==4||(argc==5&&std::string(argv[4])=="--sse2"))&&!std::ifstream(argv[3]).good(),"client-or-absent query fresh-output [--sse2]");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);gate(bool(in)&&bool(out),"Texture transform IO");
#ifdef SFM64_NATIVE_TEXTURE_TRANSFORM
    SetErrorMode(SEM_NOGPFAULTERRORBOX|SEM_FAILCRITICALERRORS);TextureTransformNativeFixture f(argv[1]);f.configure();f.word(0x854fc4,argc==5?1:0);auto handler=AddVectoredExceptionHandler(1,&lifetimeFault);unsigned cw=0,control=0;_controlfp_s(&cw,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
#else
    static_assert(sizeof(void*)==8,"Owned transform math x64");
#endif
    auto mx=_mm_getcsr();_mm_setcsr(0x1f80);std::array<uint32_t,8>q{};unsigned cases=0;
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){gate(q[0]<16,"Optional input mask");const auto before=q;MaterialMatrixMathCandidate result{};
#ifdef SFM64_NATIVE_TEXTURE_TRANSFORM
        MaterialObjectParameterStateCandidate seed;for(const char*name:{"$out","$center","$scale","$rotate","$translate"}){MaterialTypedDispatchCandidate v;v.created=true;v.name=name;v.value.numeric.typeAndMetadata=0x42;seed.material.parameters.values.push_back(v);}
        seed.material.parameters.values[1].value.numeric.vector={q[1],q[2],0,0};seed.material.parameters.values[2].value.numeric.vector={q[3],q[4],0,0};seed.material.parameters.values[3].value.numeric.vector={q[5],q[5],q[5],q[5]};seed.material.parameters.values[4].value.numeric.vector={q[6],q[7],0,0};f.prepare(seed);f.sets=f.gets=0;
        SourceMaterialPatchNodeCandidate declaration;declaration.key="TextureTransform";declaration.children={{"resultVar",1,"$out[3]"}};const char*keys[]{"centerVar","scaleVar","rotateVar","translateVar"};const char*names[]{"$center","$scale","$rotate","$translate"};for(unsigned i=0;i<4;++i)if(q[0]&(1u<<i))declaration.children.push_back({keys[i],1,names[i]});
        using Create=void*(__cdecl*)();auto*p=reinterpret_cast<Create>(0x10137e40)();gate(p&&*static_cast<uintptr_t*>(p)==0x105001a4,"Actual TextureTransform factory/vtable");using Init=bool(__thiscall*)(void*,void*,void*);gate(reinterpret_cast<Init>(0x10137ad0)(p,&f.material,f.build(declaration)),"Actual TextureTransform Init");gate(f.materialIdentity(p),"Original material identity");using Bind=void(__thiscall*)(void*,void*);reinterpret_cast<Bind>(0x10137c30)(p,nullptr);gate(f.sets==1,"One complete matrix output even with result component suffix");result=f.result;f.destroy(p);gate(f.blocks.empty(),"Original proxy cleanup");f.image.verify();
#else
        MaterialTextureTransformInputsCandidate input;if(q[0]&1)input.center=std::array<uint32_t,2>{q[1],q[2]};if(q[0]&2)input.scale=std::array<uint32_t,2>{q[3],q[4]};if(q[0]&4)input.rotation=q[5];if(q[0]&8)input.translation=std::array<uint32_t,2>{q[6],q[7]};result=materialTextureTransformMatrixCandidate(input,argc==5);
#endif
        gate(q==before,"Input query untouched");out.write(reinterpret_cast<const char*>(result.data()),64);++cases;
    }
    gate(in.eof()&&in.gcount()==0&&cases,"Texture query framing");out.flush();gate(bool(out),"Complete texture output");_mm_setcsr(mx);
#ifdef SFM64_NATIVE_TEXTURE_TRANSFORM
    _controlfp_s(&control,cw,_MCW_RC|_MCW_PC);RemoveVectoredExceptionHandler(handler);std::cout<<"native_proxy_allocations="<<f.allocations<<" releases="<<f.releases<<'\n';
#endif
    std::cout<<"PASS texture_transform_matrix_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
