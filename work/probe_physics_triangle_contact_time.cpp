#include "physics_triangle_contact_time_test_fixture.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#ifdef NATIVE_PHYSICS_TRIANGLE_CONTACT_TIME
#define NOMINMAX
#include <windows.h>
#include <float.h>
#include "native_vphysics_library_fixture.h"
static size_t active=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"contact time fault case="<<active<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("library directory input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("contact time fixture IO");
#ifdef NATIVE_PHYSICS_TRIANGLE_CONTACT_TIME
    static_assert(sizeof(void*)==4,"native x86");AddVectoredExceptionHandler(1,fault);NativeVphysicsLibraryFixture image(argv[1]);using Function=float(__thiscall*)(const void*,const float*,float);const auto function=reinterpret_cast<Function>(image.address(0x25f50));unsigned int control=0;if(_controlfp_s(&control,0,0))throw std::runtime_error("contact floating controls unavailable");std::cout<<"x87 precision="<<(control&_MCW_PC)<<'\n';
#else
    static_assert(sizeof(void*)==8,"owned AMD64");
#endif
    size_t cases=0;PhysicsTriangleContactTimeInputFixture f;
    while(in.read(reinterpret_cast<char*>(&f),sizeof(f))){const auto before=f;for(unsigned j=0;j<3;++j)for(float x:f.simplex.vertices[j].point)if(!std::isfinite(x))throw std::runtime_error("nonfinite contact point fixture");for(float x:f.direction)if(!std::isfinite(x))throw std::runtime_error("nonfinite contact direction fixture");if(!std::isfinite(f.tolerance))throw std::runtime_error("nonfinite contact tolerance fixture");
        struct Guarded{std::array<uint32_t,4>before;PhysicsTriangleContactTimeInputFixture value;std::array<uint32_t,4>after;}g;g.before.fill(0xa7b8c9da);g.after=g.before;g.value=f;float answer;
#ifdef NATIVE_PHYSICS_TRIANGLE_CONTACT_TIME
        active=cases;answer=function(&g.value.simplex,g.value.direction.data(),g.value.tolerance);
#else
        answer=sourcePhysicsTriangleContactTimeCandidate(g.value.simplex,g.value.direction,g.value.tolerance);
#endif
        for(unsigned j=0;j<4;++j)if(g.before[j]!=0xa7b8c9da||g.after[j]!=0xa7b8c9da)throw std::runtime_error("contact fixture canary");if(std::memcmp(&f,&before,sizeof(f))||std::memcmp(&g.value,&before,sizeof(f)))throw std::runtime_error("contact fixture mutation");if(!std::isfinite(answer))throw std::runtime_error("nonfinite contact answer");out.write(reinterpret_cast<const char*>(&answer),4);++cases;
    }if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("contact fixture extent");out.flush();if(!out)throw std::runtime_error("contact output write");
#ifdef NATIVE_PHYSICS_TRIANGLE_CONTACT_TIME
    image.verify();
#endif
    std::cout<<"PASS triangle contact-time cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
