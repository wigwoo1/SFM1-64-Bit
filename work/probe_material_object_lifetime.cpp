#define NOMINMAX
#include <windows.h>
#include <float.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include "material_parameter_orchestration_candidate.h"
struct CompletionAction{uint32_t index=0,operation=0;std::array<uint32_t,4>bits{};};
#include "material_fallback_native_fixture.h"

// Reference only. Actual original pool hot paths, constructor, public creation
// caller, raw reference methods, forwarding methods and destructor execute RX.
// Cache insertion and current-binding invalidation are observed service edges,
// NOT implementations of the original material cache or render context.
struct MaterialObjectLifetimeFixture:MaterialFallbackNativeFixture{
    std::array<uintptr_t,146>lifetimeMethods{};uintptr_t lifetimeObject=0;
    std::array<uintptr_t,144>queueMethods{};uintptr_t queueObject=0;
    std::array<uint8_t,0xa8+32>rawStorage{};
    std::array<uint8_t,0x1600+32>stateStorage{};
    unsigned mode=0,cacheCalls=0,unbindCalls=0,modeCalls=0,contextCalls=0,queueCalls=0;
    void*captured=nullptr;
    static MaterialObjectLifetimeFixture&self(){return *static_cast<MaterialObjectLifetimeFixture*>(active);}
    static unsigned __fastcall threadMode(void*,void*){++self().modeCalls;return self().mode;}
    static void __fastcall insert(void*object,void*,void*raw){auto&f=self();if(object!=&f.lifetimeObject||f.captured||raw!=f.raw())ExitProcess(210);f.captured=raw;++f.cacheCalls;}
    static void __fastcall unbind(void*object,void*,void*raw){auto&f=self();if(object!=&f.lifetimeObject||raw!=f.raw())ExitProcess(211);++f.unbindCalls;}
    static void* __fastcall currentContext(void*object,void*){auto&f=self();if(object!=&f.lifetimeObject)ExitProcess(212);++f.contextCalls;return &f.queueObject;}
    static void* __fastcall noCallQueue(void*object,void*){auto&f=self();if(object!=&f.queueObject)ExitProcess(213);++f.queueCalls;return nullptr;}
    static uint32_t read(const void*p,size_t offset=0){uint32_t v=0;std::memcpy(&v,static_cast<const uint8_t*>(p)+offset,4);return v;}
    void*raw(){return rawStorage.data()+16;}
    uint8_t*states(){return stateStorage.data()+16;}
    explicit MaterialObjectLifetimeFixture(const char*p,const char*d,const char*m):MaterialFallbackNativeFixture(p,d,m){
        std::copy(services.begin(),services.end(),lifetimeMethods.begin());lifetimeMethods[145]=reinterpret_cast<uintptr_t>(&unbind);
        lifetimeMethods[137]=reinterpret_cast<uintptr_t>(&insert);lifetimeMethods[142]=reinterpret_cast<uintptr_t>(&threadMode);
        lifetimeMethods[103]=reinterpret_cast<uintptr_t>(&currentContext);lifetimeObject=reinterpret_cast<uintptr_t>(lifetimeMethods.data());
        word(0xfc7b0,reinterpret_cast<uintptr_t>(&lifetimeObject));word(0x1255cc,reinterpret_cast<uintptr_t>(&lifetimeObject));
        queueMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));queueMethods[143]=reinterpret_cast<uintptr_t>(&noCallQueue);queueObject=reinterpret_cast<uintptr_t>(queueMethods.data());
        // Exact original IAT name is ??0CThreadSpinRWLock@@QAE@XZ. Resolve
        // its real private tier0 export; do not synthesize the lock constructor.
        auto ctor=GetProcAddress(tierModule,"??0CThreadSpinRWLock@@QAE@XZ");if(!ctor)throw std::runtime_error("Original symbol lock constructor");word(0x95184,reinterpret_cast<uintptr_t>(ctor));
    }
    void prepare(unsigned selected){
        if(leaseDepth||read(image.base+0x1058c8,0x10))throw std::runtime_error("Previous material still live");
        mode=selected;cacheCalls=unbindCalls=modeCalls=contextCalls=queueCalls=0;captured=nullptr;
        rawStorage.fill(0xa5);stateStorage.fill(0xa5);std::memset(raw(),0,4);std::memset(states(),0,4);
        for(auto p:{0x105868u,0x105898u,0x1058c8u})std::memset(image.base+p,0,0x28);
        word(0x1058c8,0xa8);word(0x1058d4,reinterpret_cast<uintptr_t>(raw()));
        const size_t statePool=mode&3?0x105898:0x105868;word(statePool,mode&3?0x1600:0x2c0);word(statePool+12,reinterpret_cast<uintptr_t>(states()));
    }
    void guards()const{
        auto guard=[](const auto&a,size_t n){for(size_t i=0;i<16;++i)if(a[i]!=0xa5||a[16+n+i]!=0xa5)return false;return true;};
        if(!guard(rawStorage,0xa8)||!guard(stateStorage,0x1600))throw std::runtime_error("Object/state guard changed");
    }
    void finishLifetime(){using Cleanup=void(__cdecl*)();reinterpret_cast<Cleanup>(0x10094c20)();if(read(image.base+0x13dcd4))throw std::runtime_error("Material symbols retained");finish();verifyCode();guards();}
};
static unsigned currentCase=0;
static LONG CALLBACK lifetimeFault(EXCEPTION_POINTERS*p){std::fprintf(stderr,"lifetime case=%u fault=%08lx eip=%08lx address=%08lx\n",currentCase,p->ExceptionRecord->ExceptionCode,p->ContextRecord->Eip,p->ExceptionRecord->NumberParameters>1?static_cast<unsigned long>(p->ExceptionRecord->ExceptionInformation[1]):0ul);return EXCEPTION_CONTINUE_SEARCH;}
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==4,"Private original reference only");need(argc==5,"material libraries workspace-mount fresh-output");need(!std::ifstream(argv[4]).good(),"Fresh output required");
    MaterialObjectLifetimeFixture f(argv[1],argv[2],argv[3]);auto observer=AddVectoredExceptionHandler(1,&lifetimeFault);need(observer!=nullptr,"Fault observer");
    std::ofstream out(argv[4]);need(bool(out),"Output file");unsigned checks=0,cases=0;
    auto check=[&](bool b,const char*s){need(b,s);++checks;};
    using RawFactory=void*(__cdecl*)(const char*,const char*,void*);
    using PublicFactory=void*(__thiscall*)(void*,const char*,void*);
    using Identity=void*(__thiscall*)(void*);using Name=const char*(__thiscall*)(void*);
    using Ref=void(__thiscall*)(void*);using Destroy=void*(__thiscall*)(void*,unsigned);
    const std::vector<std::pair<std::string,std::string>>names{{"Fixture/Root","fixture/root"},{"Fixture\\Mixed.VMT","fixture\\mixed"},{"{Procedural}","{procedural}"},{"",""},{std::string(255,'A'),std::string(255,'a')}};
    for(unsigned mode=0;mode<4;++mode)for(unsigned sourced=0;sourced<2;++sourced)for(unsigned api=0;api<2;++api)for(const auto&namesPair:names){
        currentCase=cases;f.prepare(mode);f.warnings=0;
        SourceMaterialPatchNodeCandidate source;source.key="UnlitGeneric";SourceMaterialPatchNodeCandidate field;field.key="$basetexture";field.kind=1;field.text="fixture/owned";source.children.push_back(field);
        void*tree=sourced?f.build(source):nullptr;const auto blockStart=f.blocks.size();
        auto handle=api?reinterpret_cast<PublicFactory>(0x1000b790)(&f.lifetimeObject,namesPair.first.c_str(),tree):reinterpret_cast<RawFactory>(0x10006cc0)(namesPair.first.c_str(),"Fixture group",tree);
        auto raw=static_cast<uint8_t*>(f.raw());void*forward=raw+0x80;
        check(handle==(api?forward:raw),"Factory return identity");check(f.read(raw)==0x10095d74&&f.read(forward)==0x10095a9c&&f.read(forward,4)==reinterpret_cast<uintptr_t>(raw),"Real vtables/backpointer");
        check(f.read(raw,0x7c)==reinterpret_cast<uintptr_t>(tree),"Constructor adopts exact source");check((f.read(raw,0x20)&0xffff)==((sourced?0x10u:0u)|(!namesPair.first.empty()&&namesPair.first[0]=='{'?0x20u:0u)),"Fresh flags");
        check(f.read(raw,0x1c)==api&&f.read(raw,0x14)==0&&f.read(raw,0x24)==0&&(f.read(raw,0x20)>>16)==0,"Initial references/no initialized shader/parameters");
        check(std::string(reinterpret_cast<Name>(0x10004190)(raw))==namesPair.second,"Normalized exact material name");check(std::string(reinterpret_cast<Name>(0x100041a0)(raw))==(api?"Other textures":"Fixture group"),"Exact texture group");
        check(reinterpret_cast<Identity>(0x100067b0)(raw)==forward&&reinterpret_cast<Identity>(0x100043d0)(forward)==raw,"Forward/raw conversion");
        check(f.cacheCalls==api&&f.captured==(api?raw:nullptr)&&f.leaseDepth==0,"Observed public cache edge/token release");
        const auto used=mode&2?0x1600u:mode&1?0x580u:0x2c0u;check(f.read(raw,0x48)==reinterpret_cast<uintptr_t>(f.states())&&f.modeCalls==2,"State pool identity/mode queries");
        check(std::all_of(f.states(),f.states()+used,[](uint8_t v){return v==0;}),"Fresh state records zeroed");check(std::all_of(f.states()+used,f.states()+0x1600,[](uint8_t v){return v==0xa5;}),"Inactive state tail untouched");
        check(f.read(raw,0x6c)==0x3e4ccccd&&f.read(raw,0x70)==0x3e4ccccd&&f.read(raw,0x74)==0x3e4ccccd,"Constructor three-component defaults (0.2)");
        if(tree){using Set=void(__thiscall*)(void*,const char*,const char*);reinterpret_cast<Set>(0x10070db0)(tree,"$basetexture","fixture/mutated");check(f.extract(reinterpret_cast<void*>(f.read(raw,0x7c))).children.front().text=="fixture/mutated","Adopted tree is not cloned");}
        const auto snapshot=f.rawStorage;
        // Through the real forwarding increment, then original direct raw
        // decrement. Refcount reaching zero must leave the object/source live.
        reinterpret_cast<Ref>(0x100075a0)(forward);check(f.read(raw,0x1c)==api+1,"Forward increment");
        reinterpret_cast<Ref>(0x1002dd90)(raw);check(f.read(raw,0x1c)==api&&f.rawStorage==snapshot,"Raw decrement restores exact object");
        if(api){reinterpret_cast<Ref>(0x10007820)(forward);check(f.read(raw,0x1c)==0&&f.contextCalls==1&&f.queueCalls==1,"Forward decrement null-queue path");}
        check(f.read(raw)==0x10095d74&&f.read(raw,0x7c)==reinterpret_cast<uintptr_t>(tree)&&f.read(f.image.base+0x1058c8,0x10)==1,"Zero references do not destroy");
        // An extra decrement demonstrates original signed underflow behavior,
        // then an increment returns to zero; no safety policy is inferred.
        reinterpret_cast<Ref>(0x1002dd90)(raw);check(f.read(raw,0x1c)==0xffffffffu&&f.read(raw)==0x10095d74,"Raw reference underflow is not deletion");reinterpret_cast<Ref>(0x10005dc0)(raw);
        const auto beforeRelease=f.releases;check(reinterpret_cast<Destroy>(0x10006d20)(raw,1)==raw,"Original scalar deletion result");
        check(f.unbindCalls==1&&f.read(raw,0x7c)==0&&f.read(raw)==reinterpret_cast<uintptr_t>(nullptr)&&f.warnings==0,"Destructor source cleanup/service edge/pool link");
        check(f.read(f.image.base+0x1058c8,0x10)==0&&f.read(f.image.base+0x1058c8,12)==reinterpret_cast<uintptr_t>(raw),"Material pool recycle");
        const auto pool=mode&3?0x105898u:0x105868u;check(f.read(f.image.base+pool,0x10)==0&&f.read(f.image.base+pool,12)==reinterpret_cast<uintptr_t>(f.states())&&f.modeCalls==4&&f.leaseDepth==0,"State recycle/token balance");
        if(tree){bool treeLive=false;for(size_t i=0;i<blockStart;++i)if(f.blocks[i].p==tree&&f.blocks[i].live)treeLive=true;check(!treeLive&&f.releases>=beforeRelease+3,"Owned source root/child/string released");}
        f.guards();f.verifyCode();out<<cases<<' '<<mode<<' '<<sourced<<' '<<api<<' '<<used<<' '<<f.cacheCalls<<' '<<f.unbindCalls<<'\n';++cases;
    }
    f.finishLifetime();out.flush();check(bool(out),"Result completion");RemoveVectoredExceptionHandler(observer);
    std::cout<<"PASS_REFERENCE_ONLY material_lifetime_cases="<<cases<<" independent_checks="<<checks<<" allocations="<<f.allocations<<" releases="<<f.releases<<"\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
