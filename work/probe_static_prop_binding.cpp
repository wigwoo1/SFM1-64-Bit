#include <Windows.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include "source_bsp_static_props_candidate.h"
#include "source_world_light_entity_filter_candidate.h"
#ifdef NATIVE_STATIC_PROP_BINDING
#include "native_private_world_image_fixture.h"
static std::array<uint8_t,256>studio{};
static const void*currentModel=nullptr;static unsigned begins=0,ends=0,boundsCalls=0,illumCalls=0,commandCalls=0;static size_t active=0;
static void __fastcall beginCache(void*,void*){++begins;}
static void __fastcall endCache(void*,void*){++ends;}
static void* __fastcall getStudio(void*,void*,const void*m){if(m!=currentModel)throw std::runtime_error("native prop model header identity");return studio.data();}
static void __fastcall getBounds(void*,void*,const void*m,float*lo,float*hi){if(m!=currentModel)throw std::runtime_error("native prop bounds identity");++boundsCalls;for(unsigned j=0;j<3;++j){lo[j]=-float(j+1);hi[j]=float(j+2);}}
static bool __fastcall getIllum(void*,void*,const void*m,const void*,const float*origin,const float*,float*out){if(m!=currentModel)throw std::runtime_error("native prop illumination identity");++illumCalls;std::memcpy(out,origin,12);return true;}
static int __fastcall checkParm(void*,void*,const char*){++commandCalls;return 0;}
static uintptr_t commandObject=0;
static void* __cdecl commandLine(){return &commandObject;}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"native prop fault case="<<active<<" eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
#endif
template<class T>static void put(std::ostream&o,const T&v){o.write(reinterpret_cast<const char*>(&v),sizeof(v));}
template<class T>static T at(const uint8_t*b,size_t p){T v;std::memcpy(&v,b+p,sizeof(v));return v;}
static std::vector<uint8_t>read(const char*p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("static props input file");return {(std::istreambuf_iterator<char>(f)),{}};}
int main(int argc,char**argv){try{
    if(argc!=5)throw std::runtime_error("engine BSP queries output required");const auto bspBytes=read(argv[2]);const SourceBspStaticPropsCandidate props(bspBytes);const auto queries=read(argv[3]);if(queries.empty()||queries.size()%72)throw std::runtime_error("static prop query extent");std::ofstream out(argv[4],std::ios::binary);if(!out)throw std::runtime_error("static prop output");
#ifdef NATIVE_STATIC_PROP_BINDING
    static_assert(sizeof(void*)==4,"original engine x86 only");AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[1]);auto base=image.base;
    std::array<uintptr_t,31>cacheTable{};cacheTable[29]=uintptr_t(&beginCache);cacheTable[30]=uintptr_t(&endCache);uintptr_t cache=uintptr_t(cacheTable.data());*reinterpret_cast<uintptr_t*>(base+0x694200)=uintptr_t(&cache);
    std::array<uintptr_t,30>modelTable{};modelTable[29]=uintptr_t(&getStudio);modelTable[7]=uintptr_t(&getBounds);modelTable[27]=uintptr_t(&getIllum);uintptr_t modelInfo=uintptr_t(modelTable.data());*reinterpret_cast<uintptr_t*>(base+0x40a5a4)=uintptr_t(&modelInfo);
    std::array<uintptr_t,11>cmdTable{};cmdTable[10]=uintptr_t(&checkParm);commandObject=uintptr_t(cmdTable.data());*reinterpret_cast<uintptr_t*>(base+0x31a4d0)=uintptr_t(&commandLine);base[0x60d610]=1;studio[0x98]=0x10;std::memset(base+0x694650,0,12);
    std::array<uint8_t,0x138>model{};currentModel=model.data();std::array<uint8_t,64>manager{};*reinterpret_cast<uint32_t*>(manager.data())=0x1035fcdc;
    std::vector<uint8_t>storage(32+65536*200,0xa7);uint8_t*objects=storage.data()+16;*reinterpret_cast<uintptr_t*>(manager.data()+0x20)=uintptr_t(objects);image.seal();
    using Init=bool(__thiscall*)(void*,int,const void*,const void*);const auto init=reinterpret_cast<Init>(base+0x1c4cf0);
    using Ptr=void*(__thiscall*)(void*);using U32=uint32_t(__thiscall*)(void*);using Index=uint32_t(__thiscall*)(void*,void*);using Resolve=void*(__thiscall*)(void*,void*);
#else
    static_assert(sizeof(void*)==8,"owned AMD64 only");
#endif
    for(size_t i=0;i<queries.size();i+=72){const uint8_t*q=queries.data()+i;const uint32_t index=at<uint32_t>(q,0);const uint64_t modelId=at<uint64_t>(q,4);SourceBspStaticPropRecordCandidate r;for(size_t j=0;j<3;++j){r.origin[j]=at<float>(q,12+j*4);r.angles[j]=at<float>(q,24+j*4);}r.solidType=q[42];r.propFlags=q[43];
        SourceStaticPropCollisionBindingCandidate binding;uint32_t decoded=0,isStatic=0,resolved=0,roundTrip=0;
#ifdef NATIVE_STATIC_PROP_BINDING
        active=i/72;if(index>65535||!modelId||(r.solidType!=0&&r.solidType!=2&&r.solidType!=6))throw std::runtime_error("native controlled prop query");std::array<uint8_t,72>record{};std::memcpy(record.data(),q+12,60);
        // Version5 normalization in101C7232..724C supplies zero at60/68 and
        // FFFFFFFF color at64 before invoking the intact initializer.
        std::memset(record.data()+64,0xff,4);const auto oldRecord=record;uint8_t*object=objects+size_t(index)*200;std::memset(object,0,200);*reinterpret_cast<uint32_t*>(object)=0x1035fa10;*reinterpret_cast<uint32_t*>(object+4)=0x1035f95c;*reinterpret_cast<uint32_t*>(object+8)=0x1035f90c;*reinterpret_cast<uint32_t*>(object+12)=0x1035f904;
        *reinterpret_cast<uint16_t*>(object+0x2c)=0xffff;*reinterpret_cast<uint16_t*>(object+0x2e)=0xffff;object[0x30]=0xff;*reinterpret_cast<uint32_t*>(object+0x3c)=0xffffffff;*reinterpret_cast<uint16_t*>(object+0x40)=0xffff;*reinterpret_cast<uint32_t*>(manager.data()+0x2c)=index+1;
        const unsigned oldIllum=illumCalls;if(!init(object,int(index),record.data(),model.data()))throw std::runtime_error("native prop initialization returned false");if(record!=oldRecord||begins!=active+1||ends!=active+1||boundsCalls!=active+1||commandCalls!=active+1||illumCalls!=oldIllum+uint32_t(!(r.propFlags&2)))throw std::runtime_error("native prop initialization services/immutability");
        uint8_t*collision=object+8;const auto vt=reinterpret_cast<uintptr_t*>(uintptr_t(at<uint32_t>(collision,0)));const auto et=reinterpret_cast<uintptr_t*>(uintptr_t(at<uint32_t>(object,0)));binding.referenceHandle=*reinterpret_cast<uint32_t*>(reinterpret_cast<Ptr>(et[2])(object));binding.solidType=reinterpret_cast<U32>(vt[11])(collision);binding.solidFlags=reinterpret_cast<U32>(vt[12])(collision);if(reinterpret_cast<Ptr>(vt[7])(collision)!=currentModel)throw std::runtime_error("native prop model getter");binding.modelId=modelId;std::memcpy(binding.origin.data(),reinterpret_cast<Ptr>(vt[8])(collision),12);std::memcpy(binding.angles.data(),reinterpret_cast<Ptr>(vt[9])(collision),12);decoded=reinterpret_cast<Index>(base+0x1c4750)(manager.data(),object);isStatic=reinterpret_cast<Index>(base+0x1c4700)(manager.data(),object);resolved=reinterpret_cast<Resolve>(base+0x1c5e70)(manager.data(),object)==collision;roundTrip=reinterpret_cast<Ptr>(vt[0])(collision)==object;
        if(at<uint16_t>(object,0x38)!=at<uint16_t>(record.data(),26)||at<uint16_t>(object,0x3a)!=at<uint16_t>(record.data(),28)||std::memcmp(object+0x10,record.data(),24))throw std::runtime_error("native record to prop fields");
#else
        binding=sourceStaticPropCollisionBindingCandidate(r,index,modelId);decoded=sourceStaticPropHandleIndexCandidate(binding.referenceHandle);isStatic=sourceStaticPropReferenceHandleCandidate(binding.referenceHandle);resolved=roundTrip=1;
#endif
        put(out,binding.referenceHandle);put(out,binding.solidType);put(out,binding.solidFlags);put(out,binding.modelId);put(out,binding.origin);put(out,binding.angles);put(out,decoded);put(out,isStatic);put(out,resolved);put(out,roundTrip);
    }
    out.flush();if(!out)throw std::runtime_error("static prop output failure");
#ifdef NATIVE_STATIC_PROP_BINDING
    for(size_t i=0;i<16;++i)if(storage[i]!=0xa7||storage[storage.size()-1-i]!=0xa7)throw std::runtime_error("native prop canary");image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS prop bindings="<<queries.size()/72<<" BSP models="<<props.models().size()<<" leaves="<<props.leaves().size()<<" props="<<props.records().size()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
