#include "ifm_renderable_bounds_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
struct BoundsDispatchFixture {
    uint32_t flags=0;IfmRenderableBoundsStateCandidate state{};IfmBoundsCandidate modelBounds{},animated{},hull{},view{};RecoveredMatrix3x4 child{},wrapper{};IfmFirstBonePoseCandidate bone{};std::vector<IfmBoundsCandidate>sequences;
};
struct BoundsDispatchServices {
    const BoundsDispatchFixture&f;std::array<uint32_t,7>events{};IfmStudioBoundsCandidate studioData;
    explicit BoundsDispatchServices(const BoundsDispatchFixture&fixture):f(fixture),studioData{(f.flags&128)!=0,f.hull,f.view,f.sequences}{}
    void synchronize(){++events[0];}
    std::optional<IfmBoundsCandidate>animatedBounds(){++events[1];if(f.flags&16)return f.animated;return {};}
    const IfmStudioBoundsCandidate*studio(){if(!(f.flags&32))++events[2];return (f.flags&(32|64))?&studioData:nullptr;}
    IfmBoundsCandidate modelBounds(){++events[3];return f.modelBounds;}
    std::optional<RecoveredMatrix3x4>childTransform(){++events[4];if(f.flags&256)return f.child;return {};}
    std::optional<IfmFirstBonePoseCandidate>firstBonePose(){++events[6];if((f.flags&(512|1024))==(512|1024))return f.bone;return {};}
    RecoveredMatrix3x4 wrapperTransform(){++events[5];return f.wrapper;}
};
#ifdef NATIVE_IFM_BOUNDS_DISPATCH
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static const BoundsDispatchFixture*fixture=nullptr;static std::array<uint32_t,7>events{};
static std::array<uint8_t,0x120>model{};static std::array<uint8_t,128>bone{};
static uint32_t studioWrapperAddress=0;static uint8_t*imageBase=nullptr;static size_t activeCase=0;
static void __fastcall synchronize(void*,void*){++events[0];}
static bool __fastcall animated(void*,void*,float*lo,float*hi){++events[1];if(!(fixture->flags&16))return false;std::memcpy(lo,fixture->animated[0].data(),12);std::memcpy(hi,fixture->animated[1].data(),12);return true;}
static void __fastcall loadStudio(void*object,void*){++events[2];if(fixture->flags&64)std::memcpy(static_cast<uint8_t*>(object)+0x70,&studioWrapperAddress,4);}
static bool __fastcall ready(void*,void*){return (fixture->flags&128)!=0;}
static void* __fastcall getModel(void*,void*){return reinterpret_cast<void*>(0x11);}
static int32_t __fastcall category(void*,void*,void*modelTag){if(modelTag!=reinterpret_cast<void*>(0x11))throw std::runtime_error("model type receiver");return fixture->state.category;}
static void __fastcall modelBounds(void*,void*,void*modelTag,float*lo,float*hi){if(modelTag!=reinterpret_cast<void*>(0x11))throw std::runtime_error("model bounds receiver");++events[3];std::memcpy(lo,fixture->modelBounds[0].data(),12);std::memcpy(hi,fixture->modelBounds[1].data(),12);}
static void __fastcall childAdjust(void*,void*,float*lo,float*hi){++events[4];if(fixture->flags&256){using Transform=void(__cdecl*)(const float*,const float*,const float*,float*,float*);reinterpret_cast<Transform>(imageBase+0x49e820)(fixture->child[0].data(),lo,hi,lo,hi);}}
static void __fastcall wrapperMatrix(void*,void*,float*out){++events[5];std::memcpy(out,fixture->wrapper.data(),48);}
static void* __fastcall resolve(void*,void*,uint32_t id){if(id==1)return fixture->state.rawModelResolved?model.data():nullptr;if(id==2){++events[6];return (fixture->flags&512)?bone.data():nullptr;}throw std::runtime_error("bounds dispatch handle");}
static bool __fastcall isA(void*self,void*,uint32_t symbol){if(self==model.data()&&symbol==0x1357)return fixture->state.gameModelTyped;if(self==bone.data()&&symbol==0x2468)return (fixture->flags&1024)!=0;throw std::runtime_error("bounds dispatch type query");}
static void redirect(uint8_t*base,size_t at,void*target){auto*p=base+at;p[0]=0xe9;const auto relative=int32_t(reinterpret_cast<intptr_t>(target)-reinterpret_cast<intptr_t>(p)-5);std::memcpy(p+1,&relative,4);}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"native bounds dispatch fault case="<<activeCase<<" code="<<std::hex<<p->ExceptionRecord->ExceptionCode<<" eip="<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
#ifndef NATIVE_IFM_BOUNDS_DISPATCH
    static_assert(sizeof(void*)==8,"owned AMD64 probe");
    if(argc==3){std::ifstream input(argv[1],std::ios::binary);if(!input)throw std::runtime_error("MDL bounds input");std::vector<uint8_t>bytes((std::istreambuf_iterator<char>(input)),{});const auto studio=readIfmMdlStudioBoundsCandidate(bytes);std::ofstream output(argv[2],std::ios::binary);uint32_t count=uint32_t(studio.sequences.size());output.write(reinterpret_cast<const char*>(&count),4);output.write(reinterpret_cast<const char*>(studio.hull.data()),24);output.write(reinterpret_cast<const char*>(studio.view.data()),24);output.write(reinterpret_cast<const char*>(studio.sequences.data()),std::streamsize(count*24));if(!output)throw std::runtime_error("MDL bounds output");std::cout<<"PASS MDL studio bounds sequences="<<count<<'\n';return 0;}
#endif
    if(argc!=4)throw std::runtime_error("ifm.dll queries.bin output.bin required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("bounds dispatch fixture IO");
#ifdef NATIVE_IFM_BOUNDS_DISPATCH
    static_assert(sizeof(void*)==4,"native x86 probe");AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[1]);imageBase=image.base;
    redirect(imageBase,0x23bfb0,reinterpret_cast<void*>(&synchronize));redirect(imageBase,0x23ef30,reinterpret_cast<void*>(&animated));redirect(imageBase,0x23b810,reinterpret_cast<void*>(&loadStudio));redirect(imageBase,0x5dac50,reinterpret_cast<void*>(&ready));redirect(imageBase,0x23bcd0,reinterpret_cast<void*>(&childAdjust));redirect(imageBase,0x23b640,reinterpret_cast<void*>(&wrapperMatrix));
    std::array<uintptr_t,19>dmTable{};dmTable[18]=reinterpret_cast<uintptr_t>(&resolve);uintptr_t dmObject=reinterpret_cast<uintptr_t>(dmTable.data()),dmAddress=reinterpret_cast<uintptr_t>(&dmObject);std::memcpy(imageBase+0xe9a51c,&dmAddress,4);
    std::array<uintptr_t,10>engineTable{};engineTable[9]=reinterpret_cast<uintptr_t>(&category);engineTable[7]=reinterpret_cast<uintptr_t>(&modelBounds);uintptr_t engineObject=reinterpret_cast<uintptr_t>(engineTable.data()),engineAddress=reinterpret_cast<uintptr_t>(&engineObject);std::memcpy(imageBase+0xfd43a8,&engineAddress,4);
    std::array<uintptr_t,9>renderTable{};renderTable[8]=reinterpret_cast<uintptr_t>(&getModel);std::array<uintptr_t,5>typeTable{};typeTable[4]=reinterpret_cast<uintptr_t>(&isA);uint32_t modelSymbol=0x1357,boneSymbol=0x2468;std::memcpy(imageBase+0xeb52d4,&modelSymbol,4);std::memcpy(imageBase+0xeb1914,&boneSymbol,4);
    std::array<uint8_t,0x34>modeObject{};const uint32_t modeAddress=reinterpret_cast<uint32_t>(modeObject.data());std::memcpy(imageBase+0xf31774,&modeAddress,4);image.seal();
    using Bounds=void(__thiscall*)(void*,float*,float*);
#endif
    std::array<uint32_t,7>header{};size_t cases=0;
    while(in.read(reinterpret_cast<char*>(header.data()),28)){
        BoundsDispatchFixture f;f.flags=header[0];if((f.flags&~2047u)||header[5]>4096||header[6]>255)throw std::runtime_error("dispatch fixture range");std::memcpy(&f.state.category,&header[1],4);std::memcpy(&f.state.sequence,&header[2],4);std::memcpy(&f.state.runtimeMode,&header[3],4);std::memcpy(&f.state.childCount,&header[4],4);f.state.modelPresent=(f.flags&1)!=0;f.state.rawModelResolved=(f.flags&2)!=0;f.state.gameModelTyped=(f.flags&4)!=0;f.state.computeBounds=(f.flags&8)!=0;f.state.renderableFlags=uint8_t(header[6]);
        std::array<float,55>values{};in.read(reinterpret_cast<char*>(values.data()),220);f.sequences.resize(header[5]);in.read(reinterpret_cast<char*>(f.sequences.data()),std::streamsize(f.sequences.size()*24));if(!in)throw std::runtime_error("dispatch fixture truncated");std::memcpy(f.modelBounds.data(),values.data(),24);std::memcpy(f.animated.data(),values.data()+6,24);std::memcpy(f.hull.data(),values.data()+12,24);std::memcpy(f.view.data(),values.data()+18,24);std::memcpy(f.child.data(),values.data()+24,48);std::memcpy(f.wrapper.data(),values.data()+36,48);std::memcpy(f.bone.position.data(),values.data()+48,12);std::memcpy(f.bone.orientation.data(),values.data()+51,16);
        const bool game=f.state.rawModelResolved&&f.state.gameModelTyped,compute=game&&(f.state.computeBounds||(f.state.renderableFlags&0x20));if(f.state.modelPresent&&f.state.category==3&&!(compute&&(f.flags&16))&&(f.flags&(32|64))&&(f.flags&128)&&f.state.sequence!=-1&&f.sequences.empty())throw std::runtime_error("dispatch fixture missing sequence zero");
#ifdef NATIVE_IFM_BOUNDS_DISPATCH
        activeCase=cases;fixture=&f;auto put=[&](auto&target,size_t at,uint32_t value){std::memcpy(target.data()+at,&value,4);};model={};bone={};put(model,0,reinterpret_cast<uint32_t>(typeTable.data()));put(bone,0,reinterpret_cast<uint32_t>(typeTable.data()));model[0xfc]=f.state.computeBounds?1:0;put(model,0x90,header[4]);const uint32_t boneHandle=2;put(model,0x10c,reinterpret_cast<uint32_t>(&boneHandle));std::memcpy(bone.data()+0x5c,f.bone.position.data(),12);std::memcpy(bone.data()+0x6c,f.bone.orientation.data(),16);
        std::vector<uint8_t>studio(0x300+f.sequences.size()*212);std::memcpy(studio.data()+0x68,f.view.data(),24);std::memcpy(studio.data()+0x80,f.hull.data(),24);put(studio,0xbc,header[5]);put(studio,0xc0,0x300);for(size_t i=0;i<f.sequences.size();++i)std::memcpy(studio.data()+0x300+i*212+32,f.sequences[i].data(),24);std::array<uint32_t,2>studioWrapper{reinterpret_cast<uint32_t>(studio.data()),0};studioWrapperAddress=reinterpret_cast<uint32_t>(studioWrapper.data());put(modeObject,0x30,header[3]);
        std::array<uint8_t,0xa0>object{};put(object,4,reinterpret_cast<uint32_t>(renderTable.data()));put(object,0x2c,1);put(object,0x34,f.state.modelPresent?0x11u:0u);put(object,0x6c,header[2]);if(f.flags&32)put(object,0x70,studioWrapperAddress);object[0x80]=f.state.renderableFlags;const auto initialObject=object;const auto modelBefore=model;const auto boneBefore=bone;const auto studioBefore=studio;
#endif
        for(unsigned wrapper=0;wrapper<2;++wrapper){IfmBoundsCandidate result{};std::array<uint32_t,7>counts{};
#ifdef NATIVE_IFM_BOUNDS_DISPATCH
            object=initialObject;events={};reinterpret_cast<Bounds>(imageBase+(wrapper?0x240520:0x23f310))(object.data()+(wrapper?4:0),result[0].data(),result[1].data());counts=events;
            auto expected=initialObject;if(events[2]&&(f.flags&64))put(expected,0x70,studioWrapperAddress);if(object!=expected||model!=modelBefore||bone!=boneBefore||studio!=studioBefore)throw std::runtime_error("native bounds dispatch input changed beyond specified cache initialization");
#else
            BoundsDispatchServices services(f);result=wrapper?ifmRenderableBoundsWrapperCandidate(f.state,services):ifmRenderableBoundsCandidate(f.state,services);counts=services.events;
#endif
            out.write(reinterpret_cast<const char*>(result.data()),24);out.write(reinterpret_cast<const char*>(counts.data()),28);
        }++cases;
    }
    if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("dispatch fixture length");out.flush();if(!out)throw std::runtime_error("dispatch fixture output");
#ifdef NATIVE_IFM_BOUNDS_DISPATCH
    image.verify();VirtualFree(imageBase,0,MEM_RELEASE);
#endif
    std::cout<<"PASS bounds dispatcher and wrapper cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
