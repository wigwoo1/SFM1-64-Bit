#include "physics_search_test_fixture.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#ifdef NATIVE_PHYSICS_SEARCH
#define NOMINMAX
#include <windows.h>
#include "native_vphysics_library_fixture.h"
#include "native_physics_search_fixture.h"
static size_t active=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"physics search fault case="<<active<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("library directory input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary),metrics(std::string(argv[3])+".stats",std::ios::binary);if(!in||!out||!metrics)throw std::runtime_error("search fixture IO");
#ifdef NATIVE_PHYSICS_SEARCH
    static_assert(sizeof(void*)==4,"native x86");AddVectoredExceptionHandler(1,fault);NativeVphysicsLibraryFixture image(argv[1]);const uint32_t defaultSurface=searchDefaultSurfaceNative(image);searchInstallTimingObserverNative(image);using Function=bool(__thiscall*)(void*);const auto function=reinterpret_cast<Function>(image.address(0x296b0));
#else
    static_assert(sizeof(void*)==8,"owned AMD64");
#endif
    PhysicsSearchInputFixture f;size_t cases=0;while(searchReadInput(in,f)){for(float x:f.state.direction)if(!std::isfinite(x))throw std::runtime_error("nonfinite search direction fixture");if(!std::isfinite(f.tolerance))throw std::runtime_error("nonfinite search tolerance fixture");std::ostringstream before(std::ios::binary);searchWriteInput(before,f);auto result=f.state;auto cache=f.shape.cache;SourcePhysicsSearchStatisticsCandidate statistics;bool answer;
#ifdef NATIVE_PHYSICS_SEARCH
        active=cases;NativeRefinementSupportFixture support(image,f.shape);NativeSupportAlignedBytesFixture self(0x90),box(0x24),ray(64);searchWriteNativeTrace(self.data()+4,f.state.trace,defaultSurface);std::memcpy(self.data()+0x58,f.state.direction.data(),12);supportWriteNative(self.data(),0x64,box.data());supportWriteNative(self.data(),0x68,support.motion.data());supportWriteNative(self.data(),0x6c,ray.data());supportWriteNative(self.data(),0x74,f.state.distance);supportWriteNative(self.data(),0x80,f.tolerance);std::memcpy(ray.data(),f.ray.data(),64);supportWriteNative(box.data(),0,reinterpret_cast<uintptr_t>(image.address(0xc8090)));std::memcpy(box.data()+4,f.box.axes.data(),24);box.data()[0x20]=uint8_t(f.box.point);const std::vector<uint8_t>oldSelf(self.data(),self.data()+self.size),oldBox(box.data(),box.data()+box.size),oldRay(ray.data(),ray.data()+ray.size);searchTimingCallsNative=0;answer=function(self.data());result=searchReadNativeState(self.data(),defaultSurface);statistics.timingCalls=searchTimingCallsNative;cache=support.readCache();for(size_t j=0;j<self.size;++j)if(!((j>=4&&j<0x64)||(j>=0x74&&j<0x78))&&self.data()[j]!=oldSelf[j])throw std::runtime_error("search changed unrelated self field");if(std::memcmp(box.data(),oldBox.data(),box.size)||std::memcmp(ray.data(),oldRay.data(),ray.size))throw std::runtime_error("search static input changed");support.verify();self.verify();box.verify();ray.verify();
#else
        answer=sourcePhysicsSearchCandidate(result,f.ray,f.box,f.shape.context,f.tolerance,cache,&statistics);
#endif
        std::ostringstream after(std::ios::binary);searchWriteInput(after,f);if(before.str()!=after.str())throw std::runtime_error("search fixture changed");searchWriteOutput(out,result,answer,cache,f.shape.context.mode);supportPut(metrics,statistics);++cases;
    }if(!cases)throw std::runtime_error("no search cases");out.flush();metrics.flush();if(!out||!metrics)throw std::runtime_error("search output write");
#ifdef NATIVE_PHYSICS_SEARCH
    image.verify();
#endif
    std::cout<<"PASS complete convex search cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
