#include "ifm_animated_hitbox_bounds_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#ifdef NATIVE_IFM_ANIMATED_BOUNDS
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static std::vector<RecoveredMatrix3x4>boneInputs;
static RecoveredMatrix3x4 worldInput{};
static uint32_t flagsInput=0,setupCalls=0,observedMax=0,observedMask=0,worldCalls=0,scaleLookups=0;
static float observedTime=0,scaleInput=1;static int32_t tickInput=0;
static std::array<uint8_t,128>model{},transform{};
static std::array<uint32_t,8>scaleAttribute{};
static bool __fastcall setup(void*,void*,void*out,int maximum,int mask,float time){++setupCalls;observedMax=uint32_t(maximum);observedMask=uint32_t(mask);observedTime=time;if(maximum!=256||mask!=0x100)throw std::runtime_error("native setup request");std::memcpy(out,boneInputs.data(),boneInputs.size()*48);return (flagsInput&32)!=0;}
static void* __fastcall modelWorld(void*,void*){++worldCalls;return worldInput.data();}
static void* __fastcall getModel(void*,void*){return nullptr;}
static bool __fastcall isModel(void*self,void*,uint32_t token){if(self!=model.data()||token!=0x1357)throw std::runtime_error("native model type query");return (flagsInput&4)!=0;}
static void* __fastcall resolve(void*,void*,uint32_t id){if(id==1)return (flagsInput&2)?model.data():nullptr;if(id==2)return (flagsInput&8)?transform.data():nullptr;throw std::runtime_error("native bounds handle");}
static void* __fastcall lookup(void*self,void*,const char*name){if(self!=transform.data()||std::strcmp(name,"scale"))throw std::runtime_error("native bounds attribute");++scaleLookups;return (flagsInput&16)?scaleAttribute.data():nullptr;}
static int32_t* __fastcall clockTick(void*,void*,int32_t*out){*out=tickInput;return out;}
static void redirect(uint8_t*base,size_t offset,void*target){auto*p=base+offset;p[0]=0xe9;const auto relative=int32_t(reinterpret_cast<intptr_t>(target)-reinterpret_cast<intptr_t>(p)-5);std::memcpy(p+1,&relative,4);}
static size_t activeCase=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"native animated bounds fault case="<<activeCase<<" code="<<std::hex<<p->ExceptionRecord->ExceptionCode<<" eip="<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
#ifndef NATIVE_IFM_ANIMATED_BOUNDS
    static_assert(sizeof(void*)==8,"owned AMD64 required");
    if(argc==3){std::ifstream input(argv[1],std::ios::binary);if(!input)throw std::runtime_error("MDL read");std::vector<uint8_t>bytes((std::istreambuf_iterator<char>(input)),{});auto boxes=readIfmMdlHitboxesCandidate(bytes);std::ofstream output(argv[2],std::ios::binary);uint32_t count=uint32_t(boxes.firstSet.size());output.write(reinterpret_cast<const char*>(&boxes.boneCount),4);output.write(reinterpret_cast<const char*>(&count),4);output.write(reinterpret_cast<const char*>(boxes.firstSet.data()),std::streamsize(count*28));if(!output)throw std::runtime_error("MDL dump write");std::cout<<"PASS MDL first-set bones="<<boxes.boneCount<<" hitboxes="<<count<<'\n';return 0;}
#endif
    if(argc!=4)throw std::runtime_error("ifm.dll queries.bin output.bin required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace bounds fixture IO");
#ifdef NATIVE_IFM_ANIMATED_BOUNDS
    static_assert(sizeof(void*)==4,"native x86 required");AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[1]);auto*base=image.base;redirect(base,0x35dd50,reinterpret_cast<void*>(&lookup));redirect(base,0x29a950,reinterpret_cast<void*>(&clockTick));
    std::array<uintptr_t,33>renderVtable{};renderVtable[8]=reinterpret_cast<uintptr_t>(&getModel);renderVtable[13]=reinterpret_cast<uintptr_t>(&setup);renderVtable[32]=reinterpret_cast<uintptr_t>(&modelWorld);
    std::array<uintptr_t,19>dmVtable{};dmVtable[18]=reinterpret_cast<uintptr_t>(&resolve);uintptr_t dmObject=reinterpret_cast<uintptr_t>(dmVtable.data()),dmAddress=reinterpret_cast<uintptr_t>(&dmObject);std::memcpy(base+0xe9a51c,&dmAddress,4);uint32_t token=0x1357;std::memcpy(base+0xeb52d4,&token,4);
    std::array<uintptr_t,5>modelVtable{};modelVtable[4]=reinterpret_cast<uintptr_t>(&isModel);const uint32_t vtable=reinterpret_cast<uint32_t>(modelVtable.data());std::memcpy(model.data(),&vtable,4);const uint32_t transformId=2;std::memcpy(model.data()+0x6c,&transformId,4);
    std::array<uint8_t,0x348>service{};uint32_t serviceAddress=reinterpret_cast<uint32_t>(service.data());std::memcpy(base+0xf32568,&serviceAddress,4);image.seal();
    using AnimatedBounds=bool(__thiscall*)(void*,float*,float*);
#endif
    std::array<uint32_t,5>header{};size_t cases=0;
    while(in.read(reinterpret_cast<char*>(header.data()),20)){
        const uint32_t flags=header[0],boneCount=header[1],boxCount=header[2];int32_t tick;float scale;std::memcpy(&tick,&header[3],4);std::memcpy(&scale,&header[4],4);if((flags&~127u)||boneCount>256||boxCount>256)throw std::runtime_error("bounds fixture count/flags");
        RecoveredMatrix3x4 world{};std::vector<RecoveredMatrix3x4>bones(boneCount);std::vector<IfmHitboxCandidate>boxes(boxCount);in.read(reinterpret_cast<char*>(world.data()),48);in.read(reinterpret_cast<char*>(bones.data()),std::streamsize(boneCount*48));in.read(reinterpret_cast<char*>(boxes.data()),std::streamsize(boxCount*28));if(!in)throw std::runtime_error("bounds fixture truncation");for(const auto&box:boxes)if(box.bone<0||uint32_t(box.bone)>=boneCount)throw std::runtime_error("fixture hitbox bone span");
        IfmBoundsCandidate result{{{11,13,17},{19,23,29}}};uint32_t has=0,calls=0,maximum=0,mask=0,worldQueries=0,lookups=0;float time=0;
#ifdef NATIVE_IFM_ANIMATED_BOUNDS
        activeCase=cases;flagsInput=flags;boneInputs=bones;worldInput=world;scaleInput=scale;tickInput=tick;setupCalls=observedMax=observedMask=worldCalls=scaleLookups=0;observedTime=0;
        scaleAttribute={};scaleAttribute[1]=reinterpret_cast<uint32_t>(&scaleInput);scaleAttribute[4]=3;
        std::array<uint8_t,128>object{};auto put=[&](auto&target,size_t at,uint32_t value){std::memcpy(target.data()+at,&value,4);};put(object,4,reinterpret_cast<uint32_t>(renderVtable.data()));put(object,0x2c,1);
        std::vector<uint8_t>studio(0x20c+boxCount*68);put(studio,0xb0,0x200);put(studio,0x204,boxCount);put(studio,0x208,12);for(uint32_t i=0;i<boxCount;++i){const auto&box=boxes[i];const size_t at=0x20c+i*68;put(studio,at,uint32_t(box.bone));std::memcpy(studio.data()+at+8,box.bounds.data(),24);}uint32_t studioAddress=reinterpret_cast<uint32_t>(studio.data());if(flags&1)put(object,0x70,reinterpret_cast<uint32_t>(&studioAddress));put(service,0x344,(flags&64)?1u:0u);
        const auto objectBefore=object,modelBefore=model,transformBefore=transform;const auto studioBefore=studio;const auto attrBefore=scaleAttribute;const auto bonesBefore=boneInputs;const auto worldBefore=worldInput;
        has=reinterpret_cast<AnimatedBounds>(base+0x23ef30)(object.data(),result[0].data(),result[1].data())?1u:0u;calls=setupCalls;maximum=observedMax;mask=observedMask;time=observedTime;worldQueries=worldCalls;lookups=scaleLookups;
        if(object!=objectBefore||model!=modelBefore||transform!=transformBefore||studio!=studioBefore||scaleAttribute!=attrBefore||std::memcmp(boneInputs.data(),bonesBefore.data(),bonesBefore.size()*48)||worldInput!=worldBefore)throw std::runtime_error("native animated bounds input changed");
#else
        const auto calculated=ifmAnimatedHitboxBoundsCandidate((flags&1)?&boxes:nullptr,(flags&64)!=0,tick,[&](auto&palette,uint32_t max,uint32_t boneMask,float seconds){++calls;maximum=max;mask=boneMask;time=seconds;std::copy(bones.begin(),bones.end(),palette.begin());return (flags&32)!=0;},[&]{++worldQueries;return world;},[&]{if((flags&14)==14){++lookups;if(flags&16)return scale;}return 1.0f;});if(calculated){has=1;result=*calculated;}
#endif
        out.write(reinterpret_cast<const char*>(&has),4);out.write(reinterpret_cast<const char*>(result.data()),24);for(auto value:{calls,maximum,mask})out.write(reinterpret_cast<const char*>(&value),4);out.write(reinterpret_cast<const char*>(&time),4);out.write(reinterpret_cast<const char*>(&worldQueries),4);out.write(reinterpret_cast<const char*>(&lookups),4);++cases;
    }
    if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("bounds fixture length");out.flush();if(!out)throw std::runtime_error("bounds fixture output");
#ifdef NATIVE_IFM_ANIMATED_BOUNDS
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS animated hitbox bounds cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
