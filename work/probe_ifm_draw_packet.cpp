#include "ifm_model_draw_packet_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
struct PacketQuery {std::array<uint32_t,7>h{};std::array<float,25>v{};};
struct PacketWire {std::array<float,36>v{};std::array<uint32_t,14>h{};};
static_assert(sizeof(PacketQuery)==128&&sizeof(PacketWire)==200,"test-only semantic wire sizes");
#if defined(NATIVE_DRAW_PACKET_IFM)||defined(NATIVE_DRAW_PACKET_ENGINE)
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static size_t activeCase=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"draw packet fault case="<<activeCase<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
template<size_t N>static void put(std::array<uint8_t,N>&a,size_t at,uint32_t x){std::memcpy(a.data()+at,&x,4);}
#endif
#ifdef NATIVE_DRAW_PACKET_IFM
static const PacketQuery*query=nullptr;static uint32_t calls=0;static std::array<uint8_t,68>captured{};static RecoveredMatrix3x4 capturedMatrix{};
static std::array<uint8_t,128>model{},transform{};static std::array<uint32_t,8>scaleAttribute{};
static void* __fastcall resolve(void*,void*,uint32_t id){if(id!=2)throw std::runtime_error("packet transform handle");return (query->h[0]&2)?transform.data():nullptr;}
static void* __fastcall lookup(void*self,void*,const char*name){if(self!=transform.data()||std::strcmp(name,"scale"))throw std::runtime_error("packet scale lookup");return (query->h[0]&4)?scaleAttribute.data():nullptr;}
static void __fastcall capture(void*,void*,const void*packet){++calls;std::memcpy(captured.data(),packet,68);uint32_t matrix;std::memcpy(&matrix,static_cast<const uint8_t*>(packet)+0x20,4);std::memcpy(capturedMatrix.data(),reinterpret_cast<const void*>(matrix),48);}
static void redirect(uint8_t*base,size_t at,void*target){auto*p=base+at;p[0]=0xe9;const int32_t delta=int32_t(reinterpret_cast<intptr_t>(target)-reinterpret_cast<intptr_t>(p)-5);std::memcpy(p+1,&delta,4);}
// The first argument intentionally occupies the original draw-flags slot8.
__declspec(naked)static void invokePacket(uint32_t,void*,void*,void*,const float*,void*,void*){__asm{
 push ebp
 mov ebp,esp
 sub esp,300h
 push ebx
 push esi
 push edi
 lea edi,[ebp-2bch]
 mov ecx,17
 mov eax,0a7a7a7a7h
 rep stosd
 mov esi,dword ptr[ebp+10h]
 mov edi,dword ptr[ebp+14h]
 mov ebx,dword ptr[ebp+18h]
 call dword ptr[ebp+0ch]
 lea esi,[ebp-2bch]
 mov edi,dword ptr[ebp+1ch]
 mov ecx,17
 rep movsd
 lea esi,[ebp-34h]
 mov edi,dword ptr[ebp+20h]
 mov ecx,12
 rep movsd
 pop edi
 pop esi
 pop ebx
 mov esp,ebp
 pop ebp
 ret
}}
#endif
#ifdef NATIVE_DRAW_PACKET_ENGINE
static uint32_t materialCalls=0,attachmentCalls=0;static int32_t observedAttachment=0;static std::array<float,3>materialPoint{};
static RecoveredMatrix3x4 engineAttachment{};static bool attachmentSuccess=false;
static bool __fastcall attachment(void*,void*,int32_t index,float*out){++attachmentCalls;observedAttachment=index;std::memcpy(out,engineAttachment.data(),48);return attachmentSuccess;}
struct PointByValue {float x,y,z;};
static void __fastcall materialOrigin(void*,void*,PointByValue p){++materialCalls;materialPoint={p.x,p.y,p.z};}
__declspec(naked)static void invokeEngine(void*,const void*,const float*,void*,float*){__asm{
 push ebp
 mov ebp,esp
 sub esp,180h
 push ebx
 push esi
 push edi
 mov eax,dword ptr[ebp+0ch]
 mov dword ptr[ebp-140h],eax
 mov ebx,dword ptr[ebp+10h]
 mov edi,dword ptr[ebp+14h]
 call dword ptr[ebp+8]
 lea esi,[ebp-150h]
 mov edi,dword ptr[ebp+18h]
 mov ecx,3
 rep movsd
 pop edi
 pop esi
 pop ebx
 mov esp,ebp
 pop ebp
 ret
}}
#endif
int main(int argc,char**argv){try{
    if(argc!=5)throw std::runtime_error("mode(ifm/engine) original.dll input.bin output.bin required");const bool ifm=std::string(argv[1])=="ifm";if(!ifm&&std::string(argv[1])!="engine")throw std::runtime_error("packet probe mode");
    std::ifstream in(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);if(!in||!out)throw std::runtime_error("packet fixture IO");
#if defined(NATIVE_DRAW_PACKET_IFM)||defined(NATIVE_DRAW_PACKET_ENGINE)
    static_assert(sizeof(void*)==4,"native x86");AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[2]);auto*base=image.base;
#else
    static_assert(sizeof(void*)==8,"owned AMD64");
#endif
#ifdef NATIVE_DRAW_PACKET_IFM
    if(!ifm)throw std::runtime_error("IFM oracle mode");redirect(base,0x35dd50,reinterpret_cast<void*>(&lookup));base[0x243b8c]=0xc3;
    // Actual original accessors: renderable+34/+40/+30/+64 correspond to base
    // position38,angles44,model34,instance68. No supplied getter answers.
    std::array<uintptr_t,32>renderTable{};renderTable[1]=reinterpret_cast<uintptr_t>(base+0x23b6c0);renderTable[2]=reinterpret_cast<uintptr_t>(base+0x23b6d0);renderTable[8]=reinterpret_cast<uintptr_t>(base+0x12c420);renderTable[31]=reinterpret_cast<uintptr_t>(base+0x23b7b0);
    std::array<uintptr_t,19>dmTable{};dmTable[18]=reinterpret_cast<uintptr_t>(&resolve);const uintptr_t dmObject=reinterpret_cast<uintptr_t>(dmTable.data()),dmAddress=reinterpret_cast<uintptr_t>(&dmObject);std::memcpy(base+0xe9a51c,&dmAddress,4);
    std::array<uintptr_t,17>drawTable{};drawTable[16]=reinterpret_cast<uintptr_t>(&capture);const uintptr_t drawObject=reinterpret_cast<uintptr_t>(drawTable.data()),drawAddress=reinterpret_cast<uintptr_t>(&drawObject);std::memcpy(base+0xf32558,&drawAddress,4);
    std::array<uint8_t,0x34>modeObject{};const uint32_t modeAddress=reinterpret_cast<uint32_t>(modeObject.data());std::memcpy(base+0xf31984,&modeAddress,4);image.seal();
#elif defined(NATIVE_DRAW_PACKET_ENGINE)
    if(ifm)throw std::runtime_error("engine oracle mode");base[0xd57fe]=0xc3;image.seal();std::array<uintptr_t,151>materialTable{};materialTable[150]=reinterpret_cast<uintptr_t>(&materialOrigin);uintptr_t materialObject=reinterpret_cast<uintptr_t>(materialTable.data());
    std::array<uintptr_t,35>attachmentTable{};attachmentTable[34]=reinterpret_cast<uintptr_t>(&attachment);uintptr_t attachmentObject=reinterpret_cast<uintptr_t>(attachmentTable.data());
    using Illumination=void(__cdecl*)(void*,const void*,const float*,float*);
#endif
    size_t cases=0;
    if(ifm){PacketQuery q;
        while(in.read(reinterpret_cast<char*>(&q),sizeof(q))){if(q.h[0]&~63u||q.h[4]>65535)throw std::runtime_error("packet query flags/instance");PacketWire wire;
#ifdef NATIVE_DRAW_PACKET_IFM
            activeCase=cases;query=&q;calls=0;captured={};put(model,0x6c,2);scaleAttribute={};scaleAttribute[1]=reinterpret_cast<uint32_t>(q.v.data()+6);scaleAttribute[4]=3;
            std::array<uint8_t,0xa0>renderable{};put(renderable,4,reinterpret_cast<uint32_t>(renderTable.data()));put(renderable,0x34,(q.h[0]&8)?0x1122:0);std::memcpy(renderable.data()+0x38,q.v.data(),24);put(renderable,0x68,q.h[4]);put(renderable,0x78,q.h[2]);put(renderable,0x7c,q.h[3]);put(modeObject,0x30,q.h[5]);
            const auto beforeObject=renderable;const auto beforeQuery=q;const auto beforeModel=model;const auto beforeTransform=transform;const auto beforeAttribute=scaleAttribute;
            std::array<uint8_t,76>raw{};raw.fill(0x5b);RecoveredMatrix3x4 matrix{};
            invokePacket(q.h[1],base+0x243a57,renderable.data()+4,(q.h[0]&1)?model.data():nullptr,(q.h[0]&16)?q.v.data()+7:nullptr,raw.data()+4,matrix.data());
            if(renderable!=beforeObject||std::memcmp(&q,&beforeQuery,sizeof(q))||model!=beforeModel||transform!=beforeTransform||scaleAttribute!=beforeAttribute)throw std::runtime_error("packet native input guard");for(unsigned i=0;i<4;++i)if(raw[i]!=0x5b||raw[72+i]!=0x5b)throw std::runtime_error("packet output guard");
            const uint8_t*p=raw.data()+4;auto word=[&](size_t at){uint32_t x;std::memcpy(&x,p+at,4);return x;};
            if(word(0x18)!=reinterpret_cast<uint32_t>(renderable.data()+4)||word(0x1c)!=((q.h[0]&8)?0x1122u:0u)||!word(0x20)||word(0x24)||word(0x28)!=((q.h[0]&16)?reinterpret_cast<uint32_t>(q.v.data()+7):0u))throw std::runtime_error("packet native pointer provenance");
            if(calls!=uint32_t(q.h[5]!=0)||(calls&&(std::memcmp(captured.data(),p,68)||std::memcmp(capturedMatrix.data(),matrix.data(),48)))||p[66]!=0xa7||p[67]!=0xa7)throw std::runtime_error("packet dispatch/padding invariant");
            std::memcpy(wire.v.data(),p,24);std::memcpy(wire.v.data()+6,matrix.data(),48);wire.v[18]=123.25f;wire.v[19]=-456.5f;wire.v[20]=789.75f;if(q.h[0]&16)std::memcpy(wire.v.data()+18,q.v.data()+7,12);std::memcpy(wire.v.data()+21,q.v.data()+10,12);
            uint16_t instanceValue;std::memcpy(&instanceValue,p+0x40,2);wire.h={1,(q.h[0]&8)?2u:0u,1,0,(q.h[0]&16)?1u:0u,word(0x2c),word(0x30),word(0x34),word(0x38),word(0x3c),instanceValue,calls,q.h[6],(q.h[0]&32)?1u:0u};
            std::memcpy(wire.v.data()+24,q.v.data()+13,48);
#elif defined(NATIVE_DRAW_PACKET_ENGINE)
            throw std::runtime_error("engine oracle cannot construct packet");
#else
            std::array<float,3>p{},a{},origin{};std::memcpy(p.data(),q.v.data(),12);std::memcpy(a.data(),q.v.data()+3,12);std::memcpy(origin.data(),q.v.data()+7,12);int32_t skin,body;std::memcpy(&skin,&q.h[2],4);std::memcpy(&body,&q.h[3],4);const float scale=(q.h[0]&7)==7?q.v[6]:1.0f;
            const auto packet=ifmModelDrawPacketCandidate(p,a,scale,1,(q.h[0]&8)?2:0,q.h[1],skin,body,uint16_t(q.h[4]),(q.h[0]&16)?std::optional<std::array<float,3>>(origin):std::nullopt);
            std::memcpy(wire.v.data(),packet.position.data(),12);std::memcpy(wire.v.data()+3,packet.angles.data(),12);std::memcpy(wire.v.data()+6,packet.modelToWorld.data(),48);wire.v[18]=123.25f;wire.v[19]=-456.5f;wire.v[20]=789.75f;if(packet.lightingOrigin)std::memcpy(wire.v.data()+18,packet.lightingOrigin->data(),12);std::memcpy(wire.v.data()+21,q.v.data()+10,12);
            uint32_t skinBits,bodyBits;std::memcpy(&skinBits,&packet.skin,4);std::memcpy(&bodyBits,&packet.body,4);wire.h={uint32_t(packet.renderable),uint32_t(packet.model),1,0,packet.lightingOrigin?1u:0u,packet.flags,uint32_t(packet.entityIndex),skinBits,bodyBits,uint32_t(packet.hitboxSet),packet.instance,uint32_t(q.h[5]!=0),q.h[6],(q.h[0]&32)?1u:0u};
            std::memcpy(wire.v.data()+24,q.v.data()+13,48);
#endif
            out.write(reinterpret_cast<const char*>(&wire),sizeof(wire));++cases;
        }
    }else{PacketWire wire;
        while(in.read(reinterpret_cast<char*>(&wire),sizeof(wire))){if(wire.h[2]!=1||wire.h[3]||wire.h[4]>1)throw std::runtime_error("invalid IFM lighting packet contract");std::array<float,3>local{},point{},defaultPoint{},delivered{};std::memcpy(local.data(),wire.v.data()+21,12);std::memcpy(point.data(),wire.v.data()+18,12);RecoveredMatrix3x4 matrix{};std::memcpy(matrix.data(),wire.v.data()+6,48);uint32_t count=0,attachmentCount=0;int32_t attachmentIndex=0,observedIndex=0;std::memcpy(&attachmentIndex,&wire.h[12],4);RecoveredMatrix3x4 attachmentMatrix{};std::memcpy(attachmentMatrix.data(),wire.v.data()+24,48);
#ifdef NATIVE_DRAW_PACKET_ENGINE
            activeCase=cases;materialCalls=0;materialPoint={};attachmentCalls=0;observedAttachment=0;engineAttachment=attachmentMatrix;attachmentSuccess=wire.h[13]!=0;std::array<uint8_t,0x200>header{},params{};std::memcpy(header.data()+0x5c,local.data(),12);put(header,0x190,0x1a0);put(header,0x1a8,wire.h[12]);put(params,0x20,reinterpret_cast<uint32_t>(matrix.data()));if(wire.h[4])put(params,0x28,reinterpret_cast<uint32_t>(point.data()));const auto beforeParams=params;const auto beforeHeader=header;const auto beforeWire=wire;const auto beforeMatrix=matrix;
            reinterpret_cast<Illumination>(base+0xd2780)(&attachmentObject,header.data(),matrix[0].data(),defaultPoint.data());invokeEngine(base+0xd5760,params.data(),defaultPoint.data(),&materialObject,delivered.data());count=materialCalls;attachmentCount=attachmentCalls;observedIndex=observedAttachment;
            if(params!=beforeParams||header!=beforeHeader||std::memcmp(&wire,&beforeWire,sizeof(wire))||matrix!=beforeMatrix||materialPoint!=delivered)throw std::runtime_error("engine packet input/material invariant");
#elif defined(NATIVE_DRAW_PACKET_IFM)
            throw std::runtime_error("IFM oracle cannot consume packet");
#else
            IfmModelDrawPacketCandidate packet;packet.modelToWorld=matrix;if(wire.h[4])packet.lightingOrigin=point;
            auto queryAttachment=[&](int32_t index,auto&outMatrix){++attachmentCount;observedIndex=index;outMatrix=attachmentMatrix;return wire.h[13]!=0;};
            delivered=ifmModelDrawLightingPointCandidate(packet,local,attachmentIndex,queryAttachment);
            // Record the default intermediate without issuing a second query.
            defaultPoint=sourceLightingTransformPointCandidate(local,(attachmentIndex>0&&wire.h[13])?attachmentMatrix:matrix);count=1;
#endif
            out.write(reinterpret_cast<const char*>(defaultPoint.data()),12);out.write(reinterpret_cast<const char*>(delivered.data()),12);
#ifdef NATIVE_DRAW_PACKET_ENGINE
            out.write(reinterpret_cast<const char*>(materialPoint.data()),12);
#else
            out.write(reinterpret_cast<const char*>(delivered.data()),12);
#endif
            out.write(reinterpret_cast<const char*>(&count),4);out.write(reinterpret_cast<const char*>(&attachmentCount),4);out.write(reinterpret_cast<const char*>(&observedIndex),4);++cases;
        }
    }
    if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("packet fixture length");out.flush();if(!out)throw std::runtime_error("packet output");
#if defined(NATIVE_DRAW_PACKET_IFM)||defined(NATIVE_DRAW_PACKET_ENGINE)
    image.verify();VirtualFree(base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS "<<(ifm?"IFM draw packet":"engine lighting consumption")<<" cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
