#pragma once
// Test-only logging/initializer services around the COMPLETE unchanged
// material-system stage. Shader-specific initialization is explicitly supplied,
// not claimed as VertexLitGeneric or any other concrete shader implementation.
#include "material_parameter_collection_native_fixture.h"
struct MaterialParameterCompletionNativeFixture:MaterialParameterCollectionNativeFixture {
    std::array<uintptr_t,5>stageMethods{};
    std::array<uint8_t,128>context{};
    std::array<uint8_t,8>emptyLog{};
    std::vector<unsigned>events,lookups;
    const std::vector<CompletionAction>*actions=nullptr;
    unsigned stageCalls=0;
    static MaterialParameterCompletionNativeFixture&self(){return *static_cast<MaterialParameterCompletionNativeFixture*>(active);}
    static void __cdecl push(int a,int b){auto&f=self();if(a!=1||b!=1)ExitProcess(120);f.events.push_back(1);}
    static void __cdecl listener(void*p){auto&f=self();if(p!=f.context.data()+0x1c)ExitProcess(121);f.events.push_back(2);}
    static void __cdecl policy(void*p){auto&f=self();if(p!=f.context.data()+0x50)ExitProcess(122);f.events.push_back(3);}
    static void __cdecl pop(int a){auto&f=self();if(a!=1)ExitProcess(123);f.events.push_back(5);}
    static void* __fastcall stageParameter(void*,void*,int index){
        auto&f=self();if(index<0||size_t(index)>=f.descriptors.size())ExitProcess(124);
        if(f.events!=std::vector<unsigned>{1,2,3,4,5})ExitProcess(129);
        f.lookups.push_back(unsigned(index));return f.descriptors[size_t(index)].data();
    }
    static void __fastcall initialize(void*,void*,void**params,const char*name){
        auto&f=self();f.events.push_back(4);++f.stageCalls;
        uint32_t word=1;uint16_t shortStatus=1;std::memcpy(&word,f.context.data()+0x54,4);std::memcpy(&shortStatus,f.context.data()+0x5a,2);
        if(word||shortStatus||std::strcmp(name,"$stage_fixture"))ExitProcess(125);
        // Must still be clear when callback runs; material-system defaults are
        // applied AFTER it, never before. Actual variable methods execute here.
        for(const auto&a:*f.actions){
            if(a.index>=f.parameters.size())ExitProcess(126);void*p=params[a.index];auto*methods=*reinterpret_cast<uintptr_t**>(p);
            if(a.operation==0){using F=void(__thiscall*)(void*,uint32_t);reinterpret_cast<F>(methods[4])(p,a.bits[0]);}
            else if(a.operation==1){using F=void(__thiscall*)(void*,uint32_t);reinterpret_cast<F>(methods[3])(p,a.bits[0]);}
            else if(a.operation==2){using F=void(__thiscall*)(void*,uint32_t,uint32_t);reinterpret_cast<F>(methods[11])(p,a.bits[0],a.bits[1]);}
            else if(a.operation==3){using F=void(__thiscall*)(void*,uint32_t,uint32_t,uint32_t);reinterpret_cast<F>(methods[10])(p,a.bits[0],a.bits[1],a.bits[2]);}
            else if(a.operation==4){using F=void(__thiscall*)(void*,uint32_t,uint32_t,uint32_t,uint32_t);reinterpret_cast<F>(methods[9])(p,a.bits[0],a.bits[1],a.bits[2],a.bits[3]);}
            else if(a.operation==6){using F=void(__thiscall*)(void*);reinterpret_cast<F>(methods[18])(p);}
            else ExitProcess(127);
        }
        word=0x12345678;std::memcpy(f.context.data()+0x54,&word,4);
    }
    explicit MaterialParameterCompletionNativeFixture(const char*path):MaterialParameterCollectionNativeFixture(path){
        std::copy(shaderMethods.begin(),shaderMethods.end(),stageMethods.begin());
        stageMethods[3]=reinterpret_cast<uintptr_t>(&stageParameter);stageMethods[4]=reinterpret_cast<uintptr_t>(&initialize);
        word(0x95148,reinterpret_cast<uintptr_t>(&push));word(0x9514c,reinterpret_cast<uintptr_t>(&listener));
        word(0x95150,reinterpret_cast<uintptr_t>(&policy));word(0x95144,reinterpret_cast<uintptr_t>(&pop));
    }
    void complete(void**params,const std::vector<uint32_t>&types,const std::vector<CompletionAction>&a){
        for(size_t i=0;i<types.size();++i){descriptors[i][2]=types[i];descriptors[i][3]=reinterpret_cast<uintptr_t>("777 deliberately unused descriptor default");}
        const auto descriptorsBefore=descriptors;
        context.fill(0);emptyLog.fill(0);const uintptr_t storage=reinterpret_cast<uintptr_t>(emptyLog.data());
        const uint32_t capacity=8,sentinel=0x87654321;const uint16_t status=0xffff;
        std::memcpy(context.data()+0x20,&storage,4);std::memcpy(context.data()+0x24,&capacity,4);context[0x35]=8;
        std::memcpy(context.data()+0x54,&sentinel,4);std::memcpy(context.data()+0x5a,&status,2);
        events.clear();lookups.clear();actions=&a;shader=reinterpret_cast<uintptr_t>(stageMethods.data());
        using Complete=void(__thiscall*)(void*,void*,void**,const char*);
        reinterpret_cast<Complete>(0x10036bb0)(context.data(),&shader,params,"$stage_fixture");
        shader=reinterpret_cast<uintptr_t>(shaderMethods.data());
        uint32_t after=1;std::memcpy(&after,context.data()+0x54,4);
        if(after||events!=std::vector<unsigned>{1,2,3,4,5}||descriptors!=descriptorsBefore)ExitProcess(128);
    }
};
