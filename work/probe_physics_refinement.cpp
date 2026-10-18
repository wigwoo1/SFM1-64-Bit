#include "physics_refinement_test_fixture.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#ifdef NATIVE_PHYSICS_REFINEMENT
#define NOMINMAX
#include <windows.h>
#include "native_vphysics_library_fixture.h"
#include "native_physics_refinement_fixture.h"
static size_t active=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"refinement fault case="<<active<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("library directory input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("refinement fixture IO");
#ifdef NATIVE_PHYSICS_REFINEMENT
    static_assert(sizeof(void*)==4,"native x86");AddVectoredExceptionHandler(1,fault);NativeVphysicsLibraryFixture image(argv[1]);using Function=float(__thiscall*)(void*,void*);const auto function=reinterpret_cast<Function>(image.address(0x28f40));
#else
    static_assert(sizeof(void*)==8,"owned AMD64");
#endif
    PhysicsRefinementInputFixture f;size_t cases=0;while(refinementReadInput(in,f)){
        std::ostringstream oldStream(std::ios::binary);refinementWriteInput(oldStream,f);if(int32_t(f.simplex.count)>=4){for(const auto&v:f.simplex.vertices)for(float x:v.point)if(!std::isfinite(x))throw std::runtime_error("nonfinite refinement simplex fixture");for(unsigned i=0;i<12;++i)if(!std::isfinite(f.ray[i]))throw std::runtime_error("nonfinite refinement ray fixture");if(!f.box.point)for(const auto&axis:f.box.axes)for(float x:axis)if(!std::isfinite(x))throw std::runtime_error("nonfinite refinement box fixture");}
        struct Guarded{std::array<uint32_t,4>before;SourcePhysicsSimplexCandidate simplex;std::array<uint32_t,4>after;}g;g.before.fill(0xa7b8c9da);g.after=g.before;g.simplex=f.simplex;auto cache=f.shape.cache;float answer;
#ifdef NATIVE_PHYSICS_REFINEMENT
        active=cases;NativeRefinementSupportFixture support(image,f.shape);NativeSupportAlignedBytesFixture self(0x70),box(0x24),ray(64);supportWriteNative(self.data(),0x64,box.data());supportWriteNative(self.data(),0x68,support.motion.data());supportWriteNative(self.data(),0x6c,ray.data());std::memcpy(ray.data(),f.ray.data(),64);supportWriteNative(box.data(),0,reinterpret_cast<uintptr_t>(image.address(0xc8090)));std::memcpy(box.data()+4,f.box.axes.data(),24);box.data()[0x20]=uint8_t(f.box.point);const std::vector<uint8_t>oldSelf(self.data(),self.data()+self.size),oldBox(box.data(),box.data()+box.size),oldRay(ray.data(),ray.data()+ray.size);answer=function(self.data(),&g.simplex);cache=support.readCache();if(std::memcmp(self.data(),oldSelf.data(),self.size)||std::memcmp(box.data(),oldBox.data(),box.size)||std::memcmp(ray.data(),oldRay.data(),ray.size))throw std::runtime_error("native refinement static inputs changed");support.verify();self.verify();box.verify();ray.verify();
#else
        answer=sourcePhysicsRefinementCandidate(g.simplex,f.ray,f.shape.context,f.box,cache);
#endif
        for(unsigned j=0;j<4;++j)if(g.before[j]!=0xa7b8c9da||g.after[j]!=0xa7b8c9da)throw std::runtime_error("refinement state canary");std::ostringstream now(std::ios::binary);refinementWriteInput(now,f);if(now.str()!=oldStream.str())throw std::runtime_error("refinement fixture changed");if(!std::isfinite(answer))throw std::runtime_error("nonfinite refinement result");refinementWriteOutput(out,g.simplex,answer,cache,f.shape.context.mode);++cases;
    }if(!cases)throw std::runtime_error("no refinement fixtures");out.flush();if(!out)throw std::runtime_error("refinement output write");
#ifdef NATIVE_PHYSICS_REFINEMENT
    image.verify();
#endif
    std::cout<<"PASS complete refinement cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
