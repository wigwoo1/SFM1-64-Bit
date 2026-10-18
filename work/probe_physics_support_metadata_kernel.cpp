#include "physics_support_metadata_test_fixture.h"
#include <fstream>
#include <iostream>
#include <iterator>
#ifdef NATIVE_PHYSICS_SUPPORT_METADATA
#define NOMINMAX
#include <windows.h>
#include <malloc.h>
#include <memory>
#include "native_vphysics_library_fixture.h"
static size_t active=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"metadata kernel fault case="<<active<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("DLL directory input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("metadata kernel IO");
#ifdef NATIVE_PHYSICS_SUPPORT_METADATA
    static_assert(sizeof(void*)==4,"original x86");AddVectoredExceptionHandler(1,fault);NativeVphysicsLibraryFixture image(argv[1]);
#else
    static_assert(sizeof(void*)==8,"owned AMD64");
#endif
    PhysicsSupportMetadataInputFixture f;size_t count=0;while(metadataReadInput(in,f)){std::ostringstream before(std::ios::binary);metadataWriteInput(before,f);SourcePhysicsSupportMetadataRecordCandidate result;
#ifdef NATIVE_PHYSICS_SUPPORT_METADATA
        active=count;const size_t bytes=16+f.convex.rawTriangles.size()*16+f.convex.rawPoints.size()*16;std::unique_ptr<uint8_t,decltype(&_aligned_free)>memory(static_cast<uint8_t*>(_aligned_malloc(bytes+32,16)),_aligned_free);if(!memory)throw std::bad_alloc();auto p=memory.get();std::memset(p,0xa7,bytes+32);uint8_t*shape=p+16;auto header=f.convex.rawLedgeHeader;header[0]=uint32_t(16+f.convex.rawTriangles.size()*16);std::memcpy(shape,header.data(),16);std::memcpy(shape+16,f.convex.rawTriangles.data(),f.convex.rawTriangles.size()*16);if(!f.convex.rawPoints.empty())std::memcpy(shape+header[0],f.convex.rawPoints.data(),f.convex.rawPoints.size()*16);std::vector<uint8_t>old(shape,shape+bytes);std::array<uint8_t,56>record;record.fill(0xa7);std::memcpy(record.data()+20,f.initial.data(),20);using Build=void(__cdecl*)(const void*,void*);reinterpret_cast<Build>(image.address(0x26450))(shape,record.data()+16);uint32_t reference;std::memcpy(&reference,record.data()+16,4);if(reference!=reinterpret_cast<uint32_t>(shape))throw std::runtime_error("metadata native ledge reference");result.ledge=f.ledge;std::memcpy(result.payload.data(),record.data()+20,20);for(size_t i=0;i<16;++i)if(p[i]!=0xa7||p[bytes+16+i]!=0xa7||record[i]!=0xa7||record[40+i]!=0xa7)throw std::runtime_error("metadata native canary");if(std::memcmp(shape,old.data(),bytes))throw std::runtime_error("metadata native shape mutated");
#else
        result=sourcePhysicsSupportMetadataCandidate(f.convex,f.ledge,f.initial);
#endif
        std::ostringstream after(std::ios::binary);metadataWriteInput(after,f);if(before.str()!=after.str())throw std::runtime_error("metadata fixture mutated");supportPut(out,result.ledge);supportPut(out,result.payload);++count;
    }if(!count)throw std::runtime_error("empty metadata input");out.flush();if(!out)throw std::runtime_error("metadata kernel output write");
#ifdef NATIVE_PHYSICS_SUPPORT_METADATA
    image.verify();
#endif
    std::cout<<"PASS metadata kernel cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
