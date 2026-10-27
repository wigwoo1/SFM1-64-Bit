#ifdef SFM64_NATIVE_WEAPON_INVIS
#define SFM64_NATIVE_OBJECT_PARAMETERS
#endif
#define SFM64_OBJECT_PARAMETER_MAIN weaponPriorParameterMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#include "material_weapon_invis_value_candidate.h"
#ifdef SFM64_NATIVE_WEAPON_INVIS
#include "material_weapon_invis_client_native_fixture.h"
#endif
int main(int argc,char**argv){try{
    gate((argc==4||argc==5)&&!std::ifstream(argv[3]).good()&&(argc!=5||!std::ifstream(argv[4]).good()),"private-client-or-absent query fresh-output [fresh-service-trace]");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);std::ofstream service;if(argc==5)service.open(argv[4],std::ios::binary);gate(bool(in)&&bool(out)&&(argc!=5||bool(service)),"Weapon contract IO");
#ifdef SFM64_NATIVE_WEAPON_INVIS
    SetErrorMode(SEM_NOGPFAULTERRORBOX|SEM_FAILCRITICALERRORS);auto handler=AddVectoredExceptionHandler(1,&lifetimeFault);MaterialWeaponInvisClientNativeFixture f(argv[1]);unsigned cw=0,control=0;_controlfp_s(&cw,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
#else
    static_assert(sizeof(void*)==8,"Independent weapon value contract x64");
#endif
    auto mx=_mm_getcsr();_mm_setcsr(0x1f80);std::array<uint32_t,11>q{};unsigned cases=0,writes=0,refused=0;
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){const auto before=q;MaterialObjectParameterStateCandidate seed;seed.material.shader="WeaponContract";MaterialTypedDispatchCandidate v;v.created=true;v.name=q[0]&1?"$cloakfactor":"$unrelated";v.value.numeric.typeAndMetadata=q[1];v.value.numeric.vector={q[10],0x3f000000,0xbf000000,0};const auto kind=q[1]&15u;gate(materialStringDomainCandidate(kind)||kind==7,"Checked initial output domain");if(kind==1)v.value.text="old";if(kind==7){MaterialMatrixMathCandidate m{};m[0]=m[5]=m[10]=m[15]=0x3f800000;v.matrix=m;v.matrixIdentity=true;}seed.material.parameters.values.push_back(v);
        bool initialized=false,identity=false;uint32_t count=0,bits=0;std::array<uint32_t,9>services{};
#ifdef SFM64_NATIVE_WEAPON_INVIS
        f.prepareTexture(seed);f.configure(q);auto*p=f.createWeapon();gate(f.weaponMaterial(p)==nullptr,"Uninitialized actual GetMaterial");SourceMaterialPatchNodeCandidate declaration;declaration.key="weapon_invis";initialized=f.initializeFunction(p,declaration);if(initialized){identity=f.weaponMaterial(p)==&f.material;gate(identity,"Actual GetMaterial owner identity");f.writes.clear();f.evaluateWeapon(p);gate(f.writes.size()<=1,"At most one actual scalar write");count=uint32_t(f.writes.size());if(count){gate(f.writes[0].index==0&&f.writes[0].operation==1,"Actual cloak SetFloat target");bits=f.writes[0].arguments[0];}}services=f.services;seed.material.parameters.values[0]=f.variables[0].value;f.destroy(p);gate(f.blocks.empty(),"Original weapon proxy cleanup");f.image.verify();
#else
        initialized=(q[0]&1)!=0;identity=initialized;auto result=materialWeaponInvisValueCandidate(q);count=bool(result);if(result){bits=*result;gate(materialTypedNumericWriteCandidate(seed.material.parameters.values[0],1,{bits,0,0,0}),"Independent scalar assignment");}
#endif
        gate(q==before,"Original query unchanged");put(out,cases);put(out,initialized);put(out,identity);put(out,count);put(out,bits);putValues(out,seed.material);if(argc==5)for(auto x:services)put(service,x);++cases;writes+=count;if(!initialized)++refused;
    }
    gate(in.eof()&&in.gcount()==0&&cases,"Complete weapon query");out.flush();if(argc==5)service.flush();gate(bool(out)&&(argc!=5||bool(service)),"Complete weapon output");_mm_setcsr(mx);
#ifdef SFM64_NATIVE_WEAPON_INVIS
    gate(f.allocations==f.releases,"Native allocation balance");_controlfp_s(&control,cw,_MCW_RC|_MCW_PC);RemoveVectoredExceptionHandler(handler);std::cout<<"native_proxy_allocations="<<f.allocations<<" releases="<<f.releases<<'\n';
#endif
    std::cout<<"PASS weapon_invis_cases="<<cases<<" writes="<<writes<<" init_refused="<<refused<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
