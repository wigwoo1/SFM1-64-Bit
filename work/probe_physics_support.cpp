#include "physics_support_test_fixture.h"
#include <fstream>
#include <iostream>
#include <iterator>
#ifdef NATIVE_PHYSICS_SUPPORT
#define NOMINMAX
#include <windows.h>
#include "native_vphysics_library_fixture.h"
#include "native_physics_support_fixture.h"
static size_t active=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"support fault case="<<active<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("library directory input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("support fixture IO");
#ifdef NATIVE_PHYSICS_SUPPORT
    static_assert(sizeof(void*)==4,"original x86");AddVectoredExceptionHandler(1,fault);NativeVphysicsLibraryFixture image(argv[1]);
#else
    static_assert(sizeof(void*)==8,"owned AMD64");
#endif
    PhysicsSupportInputFixture f;size_t count=0;while(supportReadInput(in,f)){const auto before=f;auto cache=f.cache;SourcePhysicsSupportResultCandidate result;
#ifdef NATIVE_PHYSICS_SUPPORT
        active=count;result=supportInvokeNative(image,f,cache);
#else
        result=sourcePhysicsSupportCandidate(f.context,f.direction,cache);
#endif
        std::ostringstream oldWire(std::ios::binary),newWire(std::ios::binary);supportWriteInput(oldWire,before);supportWriteInput(newWire,f);if(oldWire.str()!=newWire.str())throw std::runtime_error("support fixture mutated");for(float x:result.point)if(!std::isfinite(x))throw std::runtime_error("nonfinite support result");supportWriteOutput(out,result,cache,f.context.mode);++count;
    }if(!count)throw std::runtime_error("empty support fixtures");out.flush();if(!out)throw std::runtime_error("support output write");
#ifdef NATIVE_PHYSICS_SUPPORT
    image.verify();
#endif
    std::cout<<"PASS physics support cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
