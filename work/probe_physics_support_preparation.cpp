#include "physics_support_preparation_test_fixture.h"
#include <fstream>
#include <iostream>
#include <iterator>
#ifdef NATIVE_PHYSICS_SUPPORT_PREPARATION
#define NOMINMAX
#include <windows.h>
#include "native_vphysics_library_fixture.h"
#include "native_physics_support_preparation_fixture.h"
static size_t active=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"support prep fault case="<<active<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("library directory input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("support preparation IO");
#ifdef NATIVE_PHYSICS_SUPPORT_PREPARATION
    static_assert(sizeof(void*)==4,"native x86");AddVectoredExceptionHandler(1,fault);NativeVphysicsLibraryFixture image(argv[1]);
#else
    static_assert(sizeof(void*)==8,"owned AMD64");
#endif
    PhysicsSupportPreparationInputFixture f;size_t cases=0;while(preparationReadInput(in,f)){std::ostringstream before(std::ios::binary);preparationWriteInput(before,f);PhysicsSupportPreparationOutputFixture result;
#ifdef NATIVE_PHYSICS_SUPPORT_PREPARATION
        active=cases;result=preparationInvokeNative(image,f);
#else
        result=preparationInvokeOwned(f);
#endif
        std::ostringstream after(std::ios::binary);preparationWriteInput(after,f);if(before.str()!=after.str())throw std::runtime_error("preparation fixture changed");preparationWriteOutput(out,result);++cases;
    }if(!cases)throw std::runtime_error("empty preparation fixtures");out.flush();if(!out)throw std::runtime_error("preparation output write");
#ifdef NATIVE_PHYSICS_SUPPORT_PREPARATION
    image.verify();
#endif
    std::cout<<"PASS support preparation+selection cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
