#include "ifm_bone_precompute_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#ifdef SFM64_NATIVE_PRECOMPUTE
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
struct RefWireNative {std::array<uint8_t,32>attribute{};std::array<uint8_t,0x90>dag{};std::array<uint8_t,8>target{};uint32_t overrideHandle=UINT32_MAX,flags=0,local=0,ownerBone=0;RecoveredMatrix3x4 world{};};
static std::array<uint8_t,0x120>model{};
static std::vector<std::array<uint8_t,0x40>>transforms;
static std::vector<RefWireNative>refs;
static std::vector<uint32_t>firstRefs,boneHandles,boneFlags;
static bool gameTyped=false,modelResolved=false;
static uint32_t worldCalls=0;
template<class T>void put(T&object,size_t at,uint32_t value){std::memcpy(object.data()+at,&value,4);}
static bool __fastcall type(void*self,void*,uint32_t token){
    if(self==model.data())return token==0x2468&&gameTyped;
    for(size_t i=0;i<transforms.size();++i)if(self==transforms[i].data())return token==0x369c&&(boneFlags[i]&2);
    for(const auto&r:refs){if(self==r.dag.data())return token==0x1357&&(r.flags&4);if(self==r.target.data())return token==0x1357&&(r.flags&128);}
    return false;
}
static void* __fastcall resolve(void*,void*,uint32_t handle){
    if(handle==1)return modelResolved?model.data():nullptr;
    if(handle>=0x10000&&handle-0x10000<transforms.size()){const auto i=handle-0x10000;return (boneFlags[i]&1)?transforms[i].data():nullptr;}
    if(handle>=0x30000&&handle-0x30000<refs.size())return refs[handle-0x30000].target.data();
    if(handle==UINT32_MAX)return nullptr;throw std::runtime_error("precompute resolve handle");
}
static uint32_t __fastcall first(void*,void*,uint32_t handle){if(handle<0x10000||handle-0x10000>=firstRefs.size())throw std::runtime_error("precompute first handle");return firstRefs[handle-0x10000];}
static uint32_t __fastcall next(void*,void*,uint32_t index){if(!index||index>refs.size())throw std::runtime_error("precompute next handle");const size_t i=index-1;return i+1<refs.size()&&refs[i+1].ownerBone==refs[i].ownerBone?index+1:0;}
static void* __fastcall attribute(void*,void*,uint32_t index){if(!index||index>refs.size())throw std::runtime_error("precompute attribute handle");auto&r=refs[index-1];return (r.flags&1)?r.attribute.data():nullptr;}
static void* __fastcall lookup(void*self,void*,const char*name){if(std::strcmp(name,"overrideParent"))throw std::runtime_error("precompute attribute name");for(auto&r:refs)if(self==r.dag.data())return r.attribute.data();throw std::runtime_error("precompute lookup self");}
static void __fastcall world(void*self,void*,void*out){for(const auto&r:refs)if(self==r.dag.data()){++worldCalls;std::memcpy(out,r.world.data(),48);return;}throw std::runtime_error("precompute world self");}
static void redirect(uint8_t*base,size_t at,void*target){base[at]=0xe9;const auto delta=int32_t(reinterpret_cast<intptr_t>(target)-reinterpret_cast<intptr_t>(base+at)-5);std::memcpy(base+at+1,&delta,4);}
#endif
int main(int argc,char**argv){try{
    if(argc!=4||std::ifstream(argv[3],std::ios::binary).good())throw std::runtime_error("original.dll query fresh-output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("precompute IO");
#ifdef SFM64_NATIVE_PRECOMPUTE
    static_assert(sizeof(void*)==4,"private x86 only");NativePrivateWorldImageFixture image(argv[1]);auto*base=image.base;
    redirect(base,0x35dd50,reinterpret_cast<void*>(&lookup));redirect(base,0x4c5870,reinterpret_cast<void*>(&world));
    std::array<uintptr_t,117>dm{};dm[18]=reinterpret_cast<uintptr_t>(&resolve);dm[89]=reinterpret_cast<uintptr_t>(&attribute);dm[115]=reinterpret_cast<uintptr_t>(&first);dm[116]=reinterpret_cast<uintptr_t>(&next);const uintptr_t dmObject=reinterpret_cast<uintptr_t>(dm.data()),dmAddress=reinterpret_cast<uintptr_t>(&dmObject);std::memcpy(base+0xe9a51c,&dmAddress,4);
    const uint32_t dagToken=0x1357,gameToken=0x2468,transformToken=0x369c,referenceToken=0x5151;
    std::memcpy(base+0xeb131c,&dagToken,4);std::memcpy(base+0xeb52d4,&gameToken,4);std::memcpy(base+0xeb1914,&transformToken,4);std::memcpy(base+0xf5ed68,&referenceToken,4);base[0xf5ed6c]|=1;
    std::array<uintptr_t,5>types{};types[4]=reinterpret_cast<uintptr_t>(&type);image.seal();
#else
    static_assert(sizeof(void*)==8,"owned x64");
#endif
    size_t cases=0;std::array<uint32_t,3>header{};
    while(in.read(reinterpret_cast<char*>(header.data()),12)){
        const uint32_t count=header[0],mask=header[1],flags=header[2];if(count>256||flags>7)throw std::runtime_error("precompute header");
        RecoveredMatrix3x4 root{};std::vector<IfmBoneLocalPoseCandidate>poses(count);std::vector<IfmBoneTransformReferencesCandidate>bones(count);in.read(reinterpret_cast<char*>(root.data()),48);in.read(reinterpret_cast<char*>(poses.data()),count*40);
        for(auto&b:bones){std::array<uint32_t,2>h{};in.read(reinterpret_cast<char*>(h.data()),8);if(!in||h[0]>3||h[1]>16)throw std::runtime_error("precompute reference count");b.present=(h[0]&1)!=0;b.transformTyped=(h[0]&2)!=0;b.references.resize(h[1]);
            for(auto&r:b.references){uint32_t f=0;in.read(reinterpret_cast<char*>(&f),4);in.read(reinterpret_cast<char*>(r.world.data()),48);if(!in||f>255)throw std::runtime_error("precompute reference wire");r.attributePresent=(f&1)!=0;r.ownerPresent=(f&2)!=0;r.ownerDagTyped=(f&4)!=0;r.nameMatches=(f&8)!=0;r.sameFile=(f&16)!=0;r.overridesDisabled=(f&32)!=0;r.overridePresent=(f&64)!=0;r.overrideDagTyped=(f&128)!=0;}
        }
        const auto selected=ifmBonePrecomputeCandidate((flags&1)!=0,(flags&2)!=0,bones);
        for(size_t i=0;i<poses.size();++i)if((!(flags&1)||(flags&4))&&(poses[i].flags&mask)&&(poses[i].flags&4)&&!selected.palette[i])throw std::runtime_error("procedural bone provider unsupported");
        const auto expected=ifmBoneWorldPaletteCandidate(poses,root,mask,selected.palette);
        std::vector<RecoveredMatrix3x4>palette(count);for(auto&m:palette)for(auto&r:m)r.fill(12345.25f);
        std::vector<uint32_t>selection(count,UINT32_MAX);std::array<uint32_t,8>computed{};uint32_t calls=0;
#ifdef SFM64_NATIVE_PRECOMPUTE
        gameTyped=(flags&2)!=0;modelResolved=(flags&1)!=0;model={};put(model,0,reinterpret_cast<uint32_t>(types.data()));put(model,0x118,count);model[0x104]=(flags&4)?1:0;
        transforms=std::vector<std::array<uint8_t,0x40>>(count);boneHandles.resize(count);boneFlags.resize(count);firstRefs.assign(count,0);refs.clear();
        size_t total=0;for(const auto&b:bones)total+=b.references.size();refs.reserve(total);
        for(uint32_t i=0;i<count;++i){const auto&b=bones[i];boneFlags[i]=(b.present?1u:0u)|(b.transformTyped?2u:0u);boneHandles[i]=0x10000+i;put(transforms[i],0,reinterpret_cast<uint32_t>(types.data()));put(transforms[i],0x1c,boneHandles[i]);put(transforms[i],0x38,42);
            if(!b.references.empty())firstRefs[i]=uint32_t(refs.size()+1);
            for(uint32_t j=0;j<b.references.size();++j){const auto&r=b.references[j];RefWireNative n;n.ownerBone=i;n.local=j;n.world=r.world;n.flags=(r.attributePresent?1u:0u)|(r.ownerPresent?2u:0u)|(r.ownerDagTyped?4u:0u)|(r.nameMatches?8u:0u)|(r.sameFile?16u:0u)|(r.overridesDisabled?32u:0u)|(r.overridePresent?64u:0u)|(r.overrideDagTyped?128u:0u);refs.push_back(n);}
        }
        for(uint32_t j=0;j<refs.size();++j){auto&r=refs[j];put(r.dag,0,reinterpret_cast<uint32_t>(types.data()));put(r.target,0,reinterpret_cast<uint32_t>(types.data()));put(r.dag,0x38,(r.flags&16)?42:43);r.dag[0x64]=(r.flags&32)?1:0;
            r.overrideHandle=(r.flags&64)?0x30000+j:UINT32_MAX;put(r.attribute,4,reinterpret_cast<uint32_t>(&r.overrideHandle));put(r.attribute,8,(r.flags&2)?reinterpret_cast<uint32_t>(r.dag.data()):0);put(r.attribute,0x10,1);put(r.attribute,0x14,(r.flags&8)?referenceToken:0x9999);
        }
        put(model,0x10c,reinterpret_cast<uint32_t>(boneHandles.data()));
        if(modelResolved&&gameTyped){using Get=void*(__thiscall*)(void*,uint32_t);using Find=void*(__thiscall*)(void*);
            for(uint32_t i=0;i<count;++i){auto*t=reinterpret_cast<Get>(base+0x5c3580)(model.data(),i);if(!t)continue;const auto*d=reinterpret_cast<Find>(base+0x4d2940)(t);if(d)for(const auto&r:refs)if(d==r.dag.data())selection[i]=r.local;}
        }
        std::vector<uint8_t>studio(0x200+count*216);put(studio,0x9c,count);put(studio,0xa0,0x200);
        for(size_t i=0;i<count;++i){std::memcpy(studio.data()+0x200+i*216+4,&poses[i].parent,4);std::memcpy(studio.data()+0x200+i*216+160,&poses[i].flags,4);}
        uintptr_t studioAddress=reinterpret_cast<uintptr_t>(studio.data());std::array<uintptr_t,2>paletteWrapper{0,reinterpret_cast<uintptr_t>(palette.data())};std::array<uint8_t,0x80>object{};put(object,0x2c,1);put(object,0x70,reinterpret_cast<uint32_t>(&studioAddress));put(object,0x18,reinterpret_cast<uint32_t>(paletteWrapper.data()));
        std::vector<std::array<float,3>>positions;std::vector<std::array<float,4>>orientations;std::vector<float>scales;
        for(const auto&p:poses){positions.push_back(p.position);orientations.push_back(p.orientation);scales.push_back(p.scale);}
        const auto beforeRoot=root;const auto beforeStudio=studio;const auto beforeModel=model;const auto beforeTransforms=transforms;const auto beforeRefs=refs;const auto beforeHandles=boneHandles;worldCalls=0;
        using Kernel=void(__thiscall*)(void*,const void*,const void*,const float*,const void*,uint32_t,void*,int32_t);
        reinterpret_cast<Kernel>(base+0x2411b0)(object.data(),positions.data(),orientations.data(),scales.data(),root.data(),mask,computed.data(),0);calls=worldCalls;
        if(root!=beforeRoot||studio!=beforeStudio||model!=beforeModel||transforms!=beforeTransforms||boneHandles!=beforeHandles||std::memcmp(refs.data(),beforeRefs.data(),refs.size()*sizeof(RefWireNative)))throw std::runtime_error("precompute input mutation");
#else
        selection=selected.selectedReferences;for(uint32_t i=0;i<count;++i){if(expected[i])palette[i]=*expected[i];if(selected.palette[i]){computed[i/32]|=1u<<(i%32);++calls;}}
#endif
        out.write(reinterpret_cast<const char*>(selection.data()),count*4);out.write(reinterpret_cast<const char*>(computed.data()),32);out.write(reinterpret_cast<const char*>(&calls),4);out.write(reinterpret_cast<const char*>(palette.data()),count*48);++cases;
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("precompute fixture framing");out.flush();if(!out)throw std::runtime_error("precompute output");
#ifdef SFM64_NATIVE_PRECOMPUTE
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS precompute_cases="<<cases<<" world_provider=supplied procedural_bones=unsupported\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
