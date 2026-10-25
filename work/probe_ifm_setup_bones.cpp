#include "ifm_setup_bones_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
struct SetupFixture {
    uint32_t mode=0,flags=0,count=0,locals=0,cacheCount=0,steps=0;int32_t tick=0;
    RecoveredMatrix3x4 world{};float rootScale=1;
    std::vector<IfmBoneLocalPoseCandidate>poses;std::vector<uint32_t>typed;
    std::vector<float>flexInput,flexCache;std::vector<RecoveredMatrix3x4>cache;
};
static std::vector<uint32_t>events;
static void event(uint32_t id){events.push_back(id);}
#ifdef SFM64_NATIVE_SETUP_BONES
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static SetupFixture*f=nullptr;static uint8_t*imageBase=nullptr;
static std::array<uint8_t,0x120>model{};
static std::array<uint8_t,0x80>rootTransform{};
static std::vector<std::array<uint8_t,0x80>>transforms;
static std::vector<uint32_t>handles;
static std::vector<std::array<uint8_t,32>>scaleAttrs;
static std::array<uint8_t,32>rootScaleAttr{};
static std::vector<RecoveredMatrix3x4>*currentCache=nullptr;
static int32_t requestedMax=0;static uint32_t requestedMask=0;
static uint32_t nativeCase=0,nativeStep=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"private setup fault case="<<nativeCase<<" step="<<nativeStep<<" code="<<std::hex<<p->ExceptionRecord->ExceptionCode<<" eip="<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
template<class T>static void put(T&target,size_t at,uint32_t value){std::memcpy(target.data()+at,&value,4);}
static void __fastcall begin(void*,void*,const char*,uint32_t,const char*,uint32_t,uint32_t){event(1);}
static void __fastcall end(void*,void*){event(9);}
static void __cdecl warn(const char*,int maximum,int count){if(maximum!=requestedMax||count!=int(f->cacheCount))throw std::runtime_error("bone warning arguments");event(8);}
static int32_t* __fastcall clockTick(void*,void*,int32_t*out){event(2);*out=f->tick;return out;}
static int32_t* __fastcall clockContext(void*,void*,int32_t*out){event(7);*out=0;return out;}
static void __fastcall lock(void*,void*){event(4);}static void __fastcall unlock(void*,void*){event(6);}
static void __fastcall sync(void*,void*){event(5);}
static void* __fastcall getModel(void*,void*){return (f->flags&64)?reinterpret_cast<void*>(0x11):nullptr;}
static uint32_t studioAddress=0;
static void __fastcall loadHeader(void*self,void*){event(10);if(f->flags&128)std::memcpy(static_cast<uint8_t*>(self)+0x70,&studioAddress,4);}
static void* __fastcall resolve(void*,void*,uint32_t handle){
    if(handle==1)return (f->flags&2)?model.data():nullptr;if(handle==2)return rootTransform.data();
    if(handle>=0x10000&&handle-0x10000<transforms.size()){const auto i=handle-0x10000;return (f->typed[i]&1)?transforms[i].data():nullptr;}
    if(handle==UINT32_MAX)return nullptr;throw std::runtime_error("setup native resolve");
}
static bool __fastcall type(void*self,void*,uint32_t token){if(self==model.data())return token==0x2468&&(f->flags&4);
    for(size_t i=0;i<transforms.size();++i)if(self==transforms[i].data())return token==0x369c&&(f->typed[i]&2);return false;}
static uint32_t __fastcall firstReference(void*,void*,uint32_t){return 0;} // independently verified benchmark prepass has no eligible override
static void __fastcall modelWorld(void*self,void*,void*out){if(self!=model.data())throw std::runtime_error("setup world model");std::memcpy(out,f->world.data(),48);}
static void* __fastcall lookup(void*self,void*,const char*name){if(std::strcmp(name,"scale"))throw std::runtime_error("setup scale name");
    if(self==rootTransform.data())return (f->flags&16)?rootScaleAttr.data():nullptr;
    for(size_t i=0;i<transforms.size();++i)if(self==transforms[i].data())return (f->typed[i]&4)?scaleAttrs[i].data():nullptr;throw std::runtime_error("setup scale owner");}
static bool __fastcall prepare(void*self,void*,int maximum,uint32_t mask,int32_t tick){
    event(3);if(maximum!=requestedMax||mask!=requestedMask||tick!=INT32_MIN)throw std::runtime_error("setup dispatch arguments");
    if(f->mode){using Fn=bool(__thiscall*)(void*,int,uint32_t,int32_t);return reinterpret_cast<Fn>(imageBase+0x241820)(self,maximum,mask,tick);}
    if(f->flags&512)for(auto&m:*currentCache)m[0][3]+=1;return (f->flags&256)!=0;
}
static void redirect(uint8_t*base,size_t at,void*target){base[at]=0xe9;const auto d=int32_t(reinterpret_cast<intptr_t>(target)-reinterpret_cast<intptr_t>(base+at)-5);std::memcpy(base+at+1,&d,4);}
#else
struct SetupOwnedServices {
    SetupFixture&f;
    bool headerCached=(f.flags&1)!=0;std::vector<RecoveredMatrix3x4>wrapperCache;
    void begin(){event(1);}void end(){event(9);}bool hasClock(){return (f.flags&8)!=0;}int32_t clockTick(){event(2);return f.tick;}
    void warnCapacity(int32_t,uint32_t){event(8);}const auto&cachedPalette(){wrapperCache.assign(f.cache.begin(),f.cache.begin()+f.cacheCount);return wrapperCache;}
    bool prepare(int32_t,uint32_t,int32_t){event(3);
        if(!f.mode){if(f.flags&512)for(auto&m:f.cache)m[0][3]+=1;return (f.flags&256)!=0;}
        return ifmPrepareSetupBonesCandidate(*this);
    }
    void lock(){event(4);}void unlock(){event(6);}bool hasHeader(){return headerCached;}bool canLoadHeader(){return (f.flags&64)!=0;}
    void loadHeader(){event(10);headerCached=(f.flags&128)!=0;}
    void updateRootFrame(){lastFrame=ifmDecomposeFrameCandidate((f.flags&2)?f.world:RecoveredMatrix3x4{{{1,0,0,0},{0,1,0,0},{0,0,1,0}}});}
    bool hasTypedModel(){return (f.flags&6)==6;}void synchronize(){event(5);}uint32_t localTransformCount(){return f.locals;}
    void validateSupportedPose(){} // fixture parser rejects procedural bones; studio header has no flex drivers
    void copyFlexInputs(){ifmSetupFlexPrefixCandidate(f.flexInput,f.flexCache);}
    auto localPoses(){auto poses=f.poses;for(size_t i=0;i<poses.size();++i){if(i>=f.locals||(f.typed[i]&3)!=3){poses[i].position={};poses[i].orientation={0,0,0,1};poses[i].scale=1;}else if(!(f.typed[i]&4))poses[i].scale=1;}return poses;}
    auto modelWorld(){return f.world;}float rootScale(){return (f.flags&16)?f.rootScale:1;}void clockContext(){event(7);}
    auto precomputedPalette(){return std::vector<std::optional<RecoveredMatrix3x4>>(f.poses.size());}
    void storePalette(const std::vector<std::optional<RecoveredMatrix3x4>>&palette){for(size_t i=0;i<palette.size();++i)if(palette[i])f.cache[i]=*palette[i];}
    IfmDecomposedFrameCandidate lastFrame{};
};
#endif
int main(int argc,char**argv){try{
    if(argc!=4||std::ifstream(argv[3],std::ios::binary).good())throw std::runtime_error("ifm.dll query fresh-output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("setup IO");
#ifdef SFM64_NATIVE_SETUP_BONES
    static_assert(sizeof(void*)==4,"private x86");AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[1]);imageBase=image.base;auto*base=image.base;
    redirect(base,0x29a950,reinterpret_cast<void*>(&clockTick));redirect(base,0x295800,reinterpret_cast<void*>(&clockContext));redirect(base,0x23b810,reinterpret_cast<void*>(&loadHeader));redirect(base,0x23bfb0,reinterpret_cast<void*>(&sync));redirect(base,0x4c5870,reinterpret_cast<void*>(&modelWorld));redirect(base,0x35dd50,reinterpret_cast<void*>(&lookup));redirect(base,0xa0f1d0,reinterpret_cast<void*>(&std::memcpy));
    const uintptr_t beginFn=reinterpret_cast<uintptr_t>(&begin),endFn=reinterpret_cast<uintptr_t>(&end),warnFn=reinterpret_cast<uintptr_t>(&warn);std::memcpy(base+0xa7e460,&beginFn,4);std::memcpy(base+0xa7e48c,&endFn,4);std::memcpy(base+0xa7e464,&warnFn,4);
    std::array<uintptr_t,117>dm{};dm[18]=reinterpret_cast<uintptr_t>(&resolve);dm[115]=reinterpret_cast<uintptr_t>(&firstReference);const uintptr_t dmObject=reinterpret_cast<uintptr_t>(dm.data()),dmAddress=reinterpret_cast<uintptr_t>(&dmObject);std::memcpy(base+0xe9a51c,&dmAddress,4);
    std::array<uintptr_t,31>mdlService{};mdlService[29]=reinterpret_cast<uintptr_t>(&lock);mdlService[30]=reinterpret_cast<uintptr_t>(&unlock);const uintptr_t mdlObject=reinterpret_cast<uintptr_t>(mdlService.data()),mdlAddress=reinterpret_cast<uintptr_t>(&mdlObject);std::memcpy(base+0xf4c7fc,&mdlAddress,4);
    std::array<uint8_t,0x348>clockService{};const uintptr_t clockAddress=reinterpret_cast<uintptr_t>(clockService.data());std::memcpy(base+0xf32568,&clockAddress,4);
    const uint32_t gameToken=0x2468,transformToken=0x369c;std::memcpy(base+0xeb52d4,&gameToken,4);std::memcpy(base+0xeb1914,&transformToken,4);base[0xf5ed6c]|=1;
    std::array<uintptr_t,13>primary{};primary[12]=reinterpret_cast<uintptr_t>(&prepare);std::array<uintptr_t,9>render{};render[8]=reinterpret_cast<uintptr_t>(&getModel);std::array<uintptr_t,5>types{};types[4]=reinterpret_cast<uintptr_t>(&type);image.seal();
#else
    static_assert(sizeof(void*)==8,"owned x64");
#endif
    size_t cases=0,steps=0;std::array<uint32_t,9>h{};
    while(in.read(reinterpret_cast<char*>(h.data()),36)){
        SetupFixture fixture;fixture.mode=h[0];fixture.flags=h[1];fixture.cacheCount=h[2];fixture.count=h[3];fixture.locals=h[4];fixture.steps=h[5];
        if(h[0]>1||h[1]>1023||h[2]>256||h[3]>256||h[4]>h[3]||!h[5]||h[5]>16||h[6]>256||h[7]>256)throw std::runtime_error("setup fixture limits");std::memcpy(&fixture.tick,&h[8],4);
        in.read(reinterpret_cast<char*>(fixture.world.data()),48);in.read(reinterpret_cast<char*>(&fixture.rootScale),4);fixture.poses.resize(fixture.count);fixture.typed.resize(fixture.count);fixture.flexInput.resize(h[6]);fixture.flexCache.resize(h[7]);fixture.cache.resize(256);
        in.read(reinterpret_cast<char*>(fixture.poses.data()),fixture.count*40);in.read(reinterpret_cast<char*>(fixture.typed.data()),fixture.count*4);in.read(reinterpret_cast<char*>(fixture.flexInput.data()),h[6]*4);in.read(reinterpret_cast<char*>(fixture.flexCache.data()),h[7]*4);in.read(reinterpret_cast<char*>(fixture.cache.data()),256*48);if(!in)throw std::runtime_error("setup truncated fixture");
        for(auto t:fixture.typed)if(t>7)throw std::runtime_error("setup transform flags");
        for(float x:fixture.flexInput)if(!std::isfinite(x))throw std::runtime_error("setup nonfinite flex input unsupported");
        for(float x:fixture.flexCache)if(!std::isfinite(x))throw std::runtime_error("setup nonfinite flex cache unsupported");
        for(const auto&p:fixture.poses)if(p.flags&4)throw std::runtime_error("setup procedural fixture unsupported");
#ifdef SFM64_NATIVE_SETUP_BONES
        f=&fixture;currentCache=&fixture.cache;model={};rootTransform={};put(model,0,reinterpret_cast<uint32_t>(types.data()));put(model,0x6c,2);put(model,0x1c,1);model[0x104]=(fixture.flags&32)?1:0;put(model,0x118,fixture.locals);
        transforms=std::vector<std::array<uint8_t,0x80>>(fixture.count);scaleAttrs=std::vector<std::array<uint8_t,32>>(fixture.count);handles.resize(fixture.locals);
        for(uint32_t i=0;i<fixture.count;++i){auto&t=transforms[i];put(t,0,reinterpret_cast<uint32_t>(types.data()));put(t,0x1c,0x10000+i);std::memcpy(t.data()+0x5c,fixture.poses[i].position.data(),12);std::memcpy(t.data()+0x6c,fixture.poses[i].orientation.data(),16);put(scaleAttrs[i],0x10,3);put(scaleAttrs[i],4,reinterpret_cast<uint32_t>(&fixture.poses[i].scale));if(i<handles.size())handles[i]=0x10000+i;}
        put(model,0x10c,reinterpret_cast<uint32_t>(handles.data()));put(model,0xbc,reinterpret_cast<uint32_t>(fixture.flexInput.data()));put(model,0xc8,uint32_t(fixture.flexInput.size()));put(rootScaleAttr,0x10,3);put(rootScaleAttr,4,reinterpret_cast<uint32_t>(&fixture.rootScale));
        std::vector<uint8_t>studio(0x200+fixture.count*216);put(studio,0x9c,fixture.count);put(studio,0xa0,0x200);for(uint32_t i=0;i<fixture.count;++i){std::memcpy(studio.data()+0x200+i*216+4,&fixture.poses[i].parent,4);std::memcpy(studio.data()+0x200+i*216+160,&fixture.poses[i].flags,4);}
        uintptr_t studioPointer=reinterpret_cast<uintptr_t>(studio.data());studioAddress=reinterpret_cast<uint32_t>(&studioPointer);
        std::array<uint8_t,0xc0>object{};put(object,0,reinterpret_cast<uint32_t>(primary.data()));put(object,4,reinterpret_cast<uint32_t>(render.data()));put(object,0x18,reinterpret_cast<uint32_t>(object.data()+0x80));put(object,0x2c,1);put(object,0x70,(fixture.flags&1)?studioAddress:0);put(object,0x84,reinterpret_cast<uint32_t>(fixture.cache.data()));put(object,0x90,fixture.cacheCount);
        std::vector<float>flexStorage(256,12345.25f);std::copy(fixture.flexCache.begin(),fixture.flexCache.end(),flexStorage.begin());put(object,0xac,reinterpret_cast<uint32_t>(flexStorage.data()));put(object,0xb0,256);put(object,0xb8,uint32_t(fixture.flexCache.size()));put(object,0xbc,reinterpret_cast<uint32_t>(flexStorage.data()));put(clockService,0x344,(fixture.flags&8)?1:0);
#else
        SetupOwnedServices services{fixture};
#endif
        for(uint32_t step=0;step<fixture.steps;++step){std::array<uint32_t,4>request{};in.read(reinterpret_cast<char*>(request.data()),16);if(!in||request[3]>1)throw std::runtime_error("setup request wire");int32_t maximum;std::memcpy(&maximum,&request[0],4);events.clear();std::vector<RecoveredMatrix3x4>output(256);for(auto&m:output)for(auto&r:m)r.fill(-9876.5f);bool result;
#ifdef SFM64_NATIVE_SETUP_BONES
            const auto beforeObject=object;const auto beforeStudio=studio;const auto beforeModel=model;const auto beforeTransforms=transforms;const auto beforeHandles=handles;const auto beforeRoot=rootTransform;const auto beforeWorld=fixture.world;const auto beforePoses=fixture.poses;const auto beforeFlex=fixture.flexInput;
            nativeCase=uint32_t(cases);nativeStep=step;requestedMax=maximum;requestedMask=request[1];float time;std::memcpy(&time,&request[2],4);using Fn=bool(__thiscall*)(void*,void*,int32_t,uint32_t,float);
            result=reinterpret_cast<Fn>(imageBase+0x23ce60)(object.data()+4,request[3]?output.data():nullptr,maximum,request[1],time);
            uint32_t flexSize=0;std::memcpy(&flexSize,object.data()+0xb8,4);if(flexSize>256)throw std::runtime_error("setup flex cache overflow");fixture.flexCache.assign(flexStorage.begin(),flexStorage.begin()+flexSize);
            std::array<float,6>frame{};std::memcpy(frame.data(),object.data()+0x50,24);
            auto expectedObject=beforeObject;std::memcpy(expectedObject.data()+0x50,frame.data(),24);if((fixture.flags&128)&&std::find(events.begin(),events.end(),10u)!=events.end())put(expectedObject,0x70,studioAddress);put(expectedObject,0xb8,flexSize);
            if(object!=expectedObject||studio!=beforeStudio||model!=beforeModel||transforms!=beforeTransforms||handles!=beforeHandles||rootTransform!=beforeRoot||std::memcmp(fixture.world.data(),beforeWorld.data(),48)||std::memcmp(fixture.poses.data(),beforePoses.data(),fixture.poses.size()*40)||fixture.flexInput!=beforeFlex)throw std::runtime_error("setup unexpected input/object mutation");
#else
            result=ifmSetupBonesWrapperCandidate(maximum,request[1],request[2],request[3]?&output:nullptr,services);
            // Provider cache getter exposes only the recorded cache count.
            const std::array<float,6>frame{services.lastFrame.position[0],services.lastFrame.position[1],services.lastFrame.position[2],services.lastFrame.angles[0],services.lastFrame.angles[1],services.lastFrame.angles[2]};
#endif
            std::array<uint32_t,3>record{result?1u:0u,uint32_t(events.size()),uint32_t(fixture.flexCache.size())};out.write(reinterpret_cast<const char*>(record.data()),12);out.write(reinterpret_cast<const char*>(events.data()),std::streamsize(events.size()*4));out.write(reinterpret_cast<const char*>(frame.data()),24);out.write(reinterpret_cast<const char*>(fixture.flexCache.data()),std::streamsize(fixture.flexCache.size()*4));out.write(reinterpret_cast<const char*>(fixture.cache.data()),256*48);out.write(reinterpret_cast<const char*>(output.data()),256*48);++steps;
        }++cases;
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("setup fixture framing");out.flush();if(!out)throw std::runtime_error("setup output");
#ifdef SFM64_NATIVE_SETUP_BONES
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS setup_cases="<<cases<<" requests="<<steps<<" bone_flex_drivers=unsupported\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
