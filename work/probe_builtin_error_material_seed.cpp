#ifdef SFM64_NATIVE_BUILTIN_SEED
#define main acceptedFallbackReferenceMain
#include "probe_material_fallback_reference.cpp"
#undef main

// Capture only the original virtual CreateMaterial boundary. These opaque
// handles are fixtures, not normally constructed public IMaterial objects.
struct BuiltinSeedFixture:MaterialFallbackNativeFixture{
    std::array<uint8_t,0x2ac0>system{};
    std::array<uintptr_t,77>builderMethods{};
    std::array<uintptr_t,94>handleMethods{};
    std::array<uintptr_t,13>finishMethods{};uintptr_t finishObject=0;
    std::array<std::array<uint8_t,0x90>,10>handles{};
    std::vector<std::pair<std::string,void*>>captured;
    unsigned conversions=0,flushes=0;
    static BuiltinSeedFixture&self(){return *static_cast<BuiltinSeedFixture*>(active);}
    static void* __fastcall capture(void*object,void*,const char*name,void*source){
        auto&f=self();if(object!=f.system.data()||!name||!source||f.captured.size()>=10)ExitProcess(200);
        const auto i=f.captured.size();f.captured.emplace_back(name,source);
        auto&h=f.handles[i];h.fill(0);const auto table=reinterpret_cast<uintptr_t>(f.handleMethods.data());
        std::memcpy(h.data(),&table,4);std::memcpy(h.data()+0x7c,&source,4);return h.data();
    }
    static void* __fastcall convert(void*object,void*){
        auto&f=self();if(f.conversions>=f.captured.size()||object!=f.handles[f.conversions].data())ExitProcess(201);++f.conversions;return object;
    }
    static void __fastcall flush(void*object,void*){auto&f=self();if(object!=&f.finishObject)ExitProcess(202);++f.flushes;}
    BuiltinSeedFixture(const char*p,const char*d,const char*m):MaterialFallbackNativeFixture(p,d,m){
        captured.reserve(10);builderMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));builderMethods[76]=reinterpret_cast<uintptr_t>(&capture);
        handleMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));handleMethods[93]=reinterpret_cast<uintptr_t>(&convert);
        finishMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));finishMethods[12]=reinterpret_cast<uintptr_t>(&flush);finishObject=reinterpret_cast<uintptr_t>(finishMethods.data());
        const auto table=reinterpret_cast<uintptr_t>(builderMethods.data());std::memcpy(system.data(),&table,4);
        std::fill(system.begin()+0x2aa0,system.end(),uint8_t(0xa5));word(0x1017f8,reinterpret_cast<uintptr_t>(&finishObject));
    }
    void initialize(){using Init=void(__thiscall*)(void*);reinterpret_cast<Init>(0x1000c430)(system.data());verifyCode();}
    void releaseSources(){for(auto&c:captured)destroyTree(c.second);captured.clear();word(0x1059ec,reinterpret_cast<uintptr_t>(errorMaterial.data()));}
};

int main(int argc,char**argv){try{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    require(argc==6,"Private material/library directory/disposable mount/fresh error output/fresh builtin output required");
    require(!std::ifstream(argv[4],std::ios::binary).good()&&!std::ifstream(argv[5],std::ios::binary).good(),"Fresh outputs required");
    auto observer=AddVectoredExceptionHandler(1,&fault);BuiltinSeedFixture f(argv[1],argv[2],argv[3]);
    // Exercise the existing-initialization guard before any creation call.
    uint32_t initialized=1;std::memcpy(f.system.data()+0x2a9c,&initialized,4);const auto skipped=f.system;const auto oldError=*reinterpret_cast<uintptr_t*>(f.image.base+0x1059ec);
    f.initialize();require(f.system==skipped&&f.captured.empty()&&f.conversions==0&&f.flushes==0&&*reinterpret_cast<uintptr_t*>(f.image.base+0x1059ec)==oldError,"Guarded initializer is inert");
    initialized=0;std::memcpy(f.system.data()+0x2a9c,&initialized,4);f.initialize();
    require(f.captured.size()==10&&f.conversions==10&&f.flushes==1,"Complete builtin creation boundary");
    require(*reinterpret_cast<uintptr_t*>(f.image.base+0x1059ec)==reinterpret_cast<uintptr_t>(f.handles[0].data()),"Actual error global publication");
    require(*reinterpret_cast<uintptr_t*>(f.system.data()+0x2a9c)==reinterpret_cast<uintptr_t>(f.handles[1].data()),"Actual initialization guard publication");
    const auto complete=f.system;f.initialize();require(f.system==complete&&f.captured.size()==10&&f.conversions==10&&f.flushes==1,"Repeated initializer is inert");
    for(size_t i=0x2aa0;i<f.system.size();++i)require(f.system[i]==0xa5,"System extent guard");
    std::ofstream one(argv[4],std::ios::binary),all(argv[5],std::ios::binary);require(bool(one)&&bool(all),"Output streams");put(one,1);put(all,10);
    for(size_t i=0;i<f.captured.size();++i){auto value=f.extract(f.captured[i].second);putText(all,f.captured[i].first);putTree(all,value);if(!i){putText(one,f.captured[i].first);putTree(one,value);}}
    // Record the actual shader boundary; do not infer its initializer from the
    // existing VertexLitGeneric or Wireframe implementations.
    for(size_t i=0;i<f.shaderNames.size();++i)if(f.shaderNames[i]=="UnlitGeneric"){
        auto object=f.shaders[i];auto t=*static_cast<void***>(object);using Count=int(__thiscall*)(void*);
        std::cout<<"UNLIT parameters="<<reinterpret_cast<Count>(t[2])(object)<<" init_rva="<<std::hex<<f.relative(t[4])<<" fallback_rva="<<f.relative(t[12])<<std::dec<<'\n';
    }
    one.flush();all.flush();require(bool(one)&&bool(all),"Output completion");f.releaseSources();f.finish();f.verifyCode();RemoveVectoredExceptionHandler(observer);
    std::cout<<"PASS builtin_seed_creations=10 conversions=10 flushes=1 guard_checks=2 allocations="<<f.allocations<<" releases="<<f.releases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
#else
#define NOMINMAX
#include "builtin_error_material_seed_candidate.h"
#include "material_fallback_parameters_candidate.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
static void require(bool b,const char*m){if(!b)throw std::runtime_error(m);}
static void put(std::ostream&out,uint32_t v){out.write(reinterpret_cast<const char*>(&v),4);}
static void putText(std::ostream&out,const std::string&s){put(out,uint32_t(s.size()));out.write(s.data(),s.size());}
static void putTree(std::ostream&out,const SourceMaterialPatchNodeCandidate&n){putText(out,n.key);put(out,n.kind);put(out,n.bits);putText(out,n.text);out.write(reinterpret_cast<const char*>(&n.pointer),8);put(out,uint32_t(n.children.size()));for(const auto&c:n.children)putTree(out,c);}
static std::string snapshot(const BuiltinErrorMaterialSeedCandidate&s){std::ostringstream out;putText(out,s.name);putTree(out,s.source);return out.str();}
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"Owned seed is native x64");
    if(argc==2&&std::string(argv[1])=="--test"){
        auto first=builtinErrorMaterialSeedCandidate(),second=builtinErrorMaterialSeedCandidate();const auto original=snapshot(first);
        require(first.name=="___error.vmt"&&first.source.key=="UnlitGeneric"&&first.source.children.size()==5,"Seed identity");
        require(first.source.children[1].kind==3&&first.source.children[1].bits==0x3d4ccccd,"Original float bits");
        first.source.children[0].bits=7;first.name="changed";require(snapshot(second)==original&&snapshot(builtinErrorMaterialSeedCandidate())==original,"Independent ownership");
        SourceMaterialPatchNodeCandidate source;source.key="Wireframe";source.children.push_back({"$fallbackmaterial",1,"absent"});MaterialFallbackParametersCandidate result;result.root.key="preserved";
        require(!materialFallbackParametersCandidate(result,source,second.source,"owner",false,[](const std::string&)->std::optional<std::string>{return {};} )&&result.root.key=="preserved","UnlitGeneric remains an explicit unsupported initializer, not a fake shader substitute");
        std::cout<<"PASS builtin_seed_checks=4 unlit_parameter_path=REFUSED\n";return 0;
    }
    require(argc==3,"Fresh output and absent original-engine path required");require(!std::filesystem::exists(argv[1])&&!std::filesystem::exists(argv[2]),"Fresh output/absent runtime");
    const auto value=builtinErrorMaterialSeedCandidate();std::ofstream out(argv[1],std::ios::binary);require(bool(out),"Output stream");put(out,1);putText(out,value.name);putTree(out,value.source);out.flush();require(bool(out),"Output completion");std::cout<<"PASS owned_builtin_error_seed=1\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
#endif
