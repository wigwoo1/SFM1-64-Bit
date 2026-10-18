#include "physics_contact_plane_test_fixture.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#ifdef NATIVE_PHYSICS_CONTACT_PLANE
#define NOMINMAX
#include <windows.h>
#include "native_vphysics_library_fixture.h"
#include "native_physics_refinement_fixture.h"
#include "native_physics_contact_plane_fixture.h"
static size_t active=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"contact plane fault case="<<active<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("library directory input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("contact-plane fixture IO");
#ifdef NATIVE_PHYSICS_CONTACT_PLANE
    static_assert(sizeof(void*)==4,"native x86");AddVectoredExceptionHandler(1,fault);NativeVphysicsLibraryFixture image(argv[1]);void*function=image.address(0x24ed0);
#else
    static_assert(sizeof(void*)==8,"owned AMD64");
#endif
    PhysicsContactPlaneInputFixture f;size_t cases=0;while(contactPlaneReadInput(in,f)){if(int32_t(f.data.simplex.count)>4)throw std::runtime_error("contact plane simplex extent");std::ostringstream old(std::ios::binary);contactPlaneWriteInput(old,f);struct Guarded{std::array<uint32_t,4>before;std::array<uint8_t,84>trace;std::array<uint32_t,4>after;}g;g.before.fill(0xa7b8c9da);g.after=g.before;g.trace=f.trace;
#ifdef NATIVE_PHYSICS_CONTACT_PLANE
        active=cases;NativeRefinementSupportFixture support(image,f.data.shape);NativeSupportAlignedBytesFixture box(0x24);supportWriteNative(box.data(),0,reinterpret_cast<uintptr_t>(image.address(0xc8090)));std::memcpy(box.data()+4,f.data.box.axes.data(),24);box.data()[0x20]=uint8_t(f.data.box.point);const std::vector<uint8_t>oldBox(box.data(),box.data()+box.size);contactPlaneInvokeNative(function,box.data(),&f.data.simplex,f.data.ray.data(),support.motion.data(),g.trace.data());support.verify();box.verify();if(std::memcmp(box.data(),oldBox.data(),box.size))throw std::runtime_error("contact box mutation");const auto cache=support.readCache();if(cache.entries!=f.data.shape.cache.entries||cache.generation!=f.data.shape.cache.generation)throw std::runtime_error("contact getter changed visit cache");
#else
        contactPlaneInvokeOwned(f,g.trace);
#endif
        for(unsigned j=0;j<4;++j)if(g.before[j]!=0xa7b8c9da||g.after[j]!=0xa7b8c9da)throw std::runtime_error("contact output canary");for(size_t j=0;j<84;++j)if((j<24||j>=40)&&g.trace[j]!=f.trace[j])throw std::runtime_error("contact plane changed unrelated trace bytes");for(size_t j=24;j<40;j+=4){float x;std::memcpy(&x,g.trace.data()+j,4);if(!std::isfinite(x))throw std::runtime_error("nonfinite contact result");}std::ostringstream now(std::ios::binary);contactPlaneWriteInput(now,f);if(now.str()!=old.str())throw std::runtime_error("contact input mutation");out.write(reinterpret_cast<const char*>(g.trace.data()),84);++cases;
    }if(!cases)throw std::runtime_error("no contact-plane fixtures");out.flush();if(!out)throw std::runtime_error("contact output write");
#ifdef NATIVE_PHYSICS_CONTACT_PLANE
    image.verify();
#endif
    std::cout<<"PASS contact plane cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
