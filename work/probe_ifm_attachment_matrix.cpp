#include "ifm_attachment_matrix_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
struct Query {std::array<uint32_t,8>h{};std::array<float,30>v{};};
static_assert(sizeof(Query)==152,"test query size");
#ifdef NATIVE_IFM_ATTACHMENT_MATRIX
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static std::array<uint32_t,7>events{};static int32_t tick=0;static uint32_t setupResult=0;
static std::array<uint8_t,92>descriptor{};
static void*__fastcall noModel(void*,void*){return nullptr;}
static int32_t*__fastcall clockTick(void*,void*,int32_t*out){*out=tick;return out;}
static bool __fastcall setup(void*,void*,void*out,int32_t maximum,uint32_t mask,float time){
    if(out)throw std::runtime_error("attachment setup output must be null");++events[0];events[1]=uint32_t(maximum);events[2]=mask;std::memcpy(&events[3],&time,4);return setupResult!=0;
}
static void*__fastcall getDescriptor(void*,void*,uint32_t index){++events[4];events[6]=index;return descriptor.data();}
static uint32_t __fastcall getBone(void*,void*,uint32_t){++events[5];return 0;}
static void redirect(uint8_t*base,size_t at,void*target){auto*p=base+at;*p=0xe9;const int32_t d=int32_t(reinterpret_cast<intptr_t>(target)-reinterpret_cast<intptr_t>(p)-5);std::memcpy(p+1,&d,4);}
#endif
int main(int argc,char**argv){try{
    if(argc!=4||std::ifstream(argv[3],std::ios::binary).good())throw std::runtime_error("original.dll queries.bin fresh-output.bin required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("attachment IO");
#ifdef NATIVE_IFM_ATTACHMENT_MATRIX
    static_assert(sizeof(void*)==4,"x86 oracle");NativePrivateWorldImageFixture image(argv[1]);
    redirect(image.base,0x29a950,reinterpret_cast<void*>(&clockTick));redirect(image.base,0x5da640,reinterpret_cast<void*>(&getDescriptor));redirect(image.base,0x5da6a0,reinterpret_cast<void*>(&getBone));
    std::array<uintptr_t,35>table{};table[1]=0x1023b6c0;table[2]=0x1023b6d0;table[8]=reinterpret_cast<uintptr_t>(&noModel);table[13]=reinterpret_cast<uintptr_t>(&setup);
    std::array<uint8_t,0x348>clock{};*reinterpret_cast<uintptr_t*>(clock.data()+0x344)=1;*reinterpret_cast<uintptr_t*>(image.base+0xf32568)=reinterpret_cast<uintptr_t>(clock.data());image.seal();
#else
    static_assert(sizeof(void*)==8,"owned x64");
#endif
    size_t cases=0;Query q;
    while(in.read(reinterpret_cast<char*>(&q),sizeof(q))){
        if(q.h[0]>1||q.h[2]>1||q.h[6]>1)throw std::runtime_error("query flags");int32_t index=0,localCount=0,virtualCount=0,clockValue=0;std::memcpy(&index,&q.h[1],4);std::memcpy(&localCount,&q.h[3],4);std::memcpy(&virtualCount,&q.h[4],4);std::memcpy(&clockValue,&q.h[7],4);
        std::array<float,3>position{},angles{};std::memcpy(position.data(),q.v.data(),12);std::memcpy(angles.data(),q.v.data()+3,12);
        IfmAttachmentDescriptorCandidate desc;desc.flags=q.h[5];std::memcpy(desc.local.data(),q.v.data()+6,48);RecoveredMatrix3x4 bone{};std::memcpy(bone.data(),q.v.data()+18,48);
        std::array<uint32_t,7>counts{};RecoveredMatrix3x4 matrix{};uint32_t valid=0;
#ifdef NATIVE_IFM_ATTACHMENT_MATRIX
        events={};tick=clockValue;setupResult=q.h[6];descriptor={};std::memcpy(descriptor.data()+4,&q.h[5],4);std::memcpy(descriptor.data()+12,desc.local.data(),48);
        std::array<uint8_t,0x200>header{},virtualModel{};std::memcpy(header.data()+0xf0,&localCount,4);std::memcpy(virtualModel.data()+0x3c,&virtualCount,4);
        std::array<uintptr_t,2>wrapper{reinterpret_cast<uintptr_t>(header.data()),q.h[2]?reinterpret_cast<uintptr_t>(virtualModel.data()):0};
        std::array<uintptr_t,2>palette{0,reinterpret_cast<uintptr_t>(bone.data())};std::array<uint8_t,0xa0>object{};
        const auto tableAddress=reinterpret_cast<uintptr_t>(table.data()),wrapperAddress=reinterpret_cast<uintptr_t>(wrapper.data()),paletteAddress=reinterpret_cast<uintptr_t>(palette.data());
        std::memcpy(object.data()+4,&tableAddress,4);if(q.h[0])std::memcpy(object.data()+0x70,&wrapperAddress,4);std::memcpy(object.data()+0x18,&paletteAddress,4);std::memcpy(object.data()+0x38,position.data(),12);std::memcpy(object.data()+0x44,angles.data(),12);
        const auto before=object;const auto beforeHeader=header,beforeVirtual=virtualModel;const auto beforeBone=bone;const auto beforeDescriptor=descriptor;
        std::array<uint32_t,14>guard{};guard.front()=guard.back()=0x7a915bc3;
        using QueryAttachment=bool(__thiscall*)(void*,int32_t,void*);valid=reinterpret_cast<QueryAttachment>(image.base+0x23c3d0)(object.data()+4,index,guard.data()+1);std::memcpy(matrix.data(),guard.data()+1,48);counts=events;
        if(object!=before||header!=beforeHeader||virtualModel!=beforeVirtual||bone!=beforeBone||descriptor!=beforeDescriptor||guard.front()!=0x7a915bc3||guard.back()!=0x7a915bc3)throw std::runtime_error("attachment mutation/canary");
#else
        struct Services {const Query&q;int32_t tick,localCount,virtualCount;const std::array<float,3>&p,&a;IfmAttachmentDescriptorCandidate d;RecoveredMatrix3x4 bone;std::array<uint32_t,7>&counts;
            bool headerAvailable(){return q.h[0]!=0;}int32_t attachmentCount(){return q.h[2]?virtualCount:localCount;}auto position(){return p;}auto angles(){return a;}int32_t clockTick(){return tick;}
            bool setupBones(uint32_t maximum,uint32_t mask,float time){++counts[0];counts[1]=maximum;counts[2]=mask;std::memcpy(&counts[3],&time,4);return q.h[6]!=0;}
            auto descriptor(uint32_t i){++counts[4];counts[6]=i;return d;}uint32_t mappedBone(uint32_t){++counts[5];return 0;}auto boneWorld(uint32_t){return bone;}
        }services{q,clockValue,localCount,virtualCount,position,angles,desc,bone,counts};
        const auto result=ifmAttachmentMatrixCandidate(index,services);valid=result.valid;matrix=result.matrix;
#endif
        out.write(reinterpret_cast<const char*>(&valid),4);out.write(reinterpret_cast<const char*>(matrix.data()),48);out.write(reinterpret_cast<const char*>(counts.data()),28);++cases;
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("attachment query shape");out.flush();if(!out)throw std::runtime_error("attachment output");
#ifdef NATIVE_IFM_ATTACHMENT_MATRIX
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS attachment_cases="<<cases<<" virtual_mapping=controlled\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
