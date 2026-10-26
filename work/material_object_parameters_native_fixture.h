#pragma once
#define SFM64_QUEUED_MATERIAL_LIFETIME_FIXTURE_ONLY
#include "probe_queued_material_lifetime.cpp"
#include "builtin_error_material_seed_candidate.h"

// Reference only: normally constructed CMaterial/raw+forward identity, actual
// dictionary, wrapper62, variable constructors/defaults, source load/fallback,
// getters, forwarding snapshots, uncache/destructor and queue execute RX.
// No proxy factory is supplied; initialized GPU resources are outside this gate.
struct MaterialObjectParametersNativeFixture:QueuedMaterialLifetimeFixture {
    using QueuedMaterialLifetimeFixture::QueuedMaterialLifetimeFixture;
    static void* __fastcall noProxyFactory(void*,void*){return nullptr;}
    void beginParameters(){start();lifetimeMethods[60]=reinterpret_cast<uintptr_t>(&noProxyFactory);}
    void configure(const MaterialParameterCollectionEnvironmentCandidate&env,const VertexLitInitSettingsCandidate&s,int32_t hardwareLevel){
        MaterialParameterCollectionNativeFixture::prepare(env,{}, {},true);
        auto&ctx=MaterialParameterCompletionNativeFixture::context;ctx.fill(0);emptyLog.fill(0);const auto table=reinterpret_cast<uintptr_t>(systemMethods.data()),storage=reinterpret_cast<uintptr_t>(emptyLog.data());const uint32_t cap=8;
        std::memcpy(ctx.data(),&table,4);std::memcpy(ctx.data()+0x20,&storage,4);std::memcpy(ctx.data()+0x24,&cap,4);ctx[0x35]=8;word(0x1017f8,reinterpret_cast<uintptr_t>(ctx.data()));
        config.fill(0);phong.fill(0);fog.fill(0);config[13]=s.configFlagsAt34;phong[12]=uint32_t(s.matPhong);fog[12]=uint32_t(s.forceVertexFog);level=hardwareLevel;tools=s.toolsSwitchPresent;stdBase[0xbebdc]=0;stdBase[0xbebd8]=0xa5;
        events.clear();lookups.clear();iterations=shaderWarnings=developerMessages=0;
    }
    uint8_t*createActual(const char*name,void*source,bool cached=true){
        if(cached){using F=void*(__thiscall*)(void*,const char*,void*);return static_cast<uint8_t*>(reinterpret_cast<F>(0x1000b790)(system(),name,source))-0x80;}
        using F=void*(__cdecl*)(const char*,const char*,void*);return static_cast<uint8_t*>(reinterpret_cast<F>(0x10006cc0)(name,"Fixture group",source));
    }
    bool initializeActual(void*raw,void*external=nullptr,void*patch=nullptr){
        auto t=*static_cast<uintptr_t**>(raw);if(t[62]!=0x10007360)throw std::runtime_error("Real material wrapper62 required");
        using F=bool(__thiscall*)(void*,void*,void*,void*);return reinterpret_cast<F>(t[62])(raw,external,patch,nullptr);
    }
    void uncacheActual(void*raw,bool preserve=false){using F=void(__thiscall*)(void*,bool);reinterpret_cast<F>(0x10005d10)(raw,preserve);}
    void destroyActual(void*raw){using F=void(__thiscall*)(void*,void*);reinterpret_cast<F>(0x10009b80)(dictionary(),raw);}
    void finishParameters(){stop();finishLifetime();}
};
