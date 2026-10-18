#include "physics_line_simplex_test_fixture.h"
#include "source_physics_triangle_simplex_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#ifdef NATIVE_PHYSICS_TRIANGLE_SIMPLEX
#define NOMINMAX
#include <windows.h>
#include "native_vphysics_library_fixture.h"
static size_t active=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"simplex fault case="<<active<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("library directory input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("simplex fixture IO");
#ifdef NATIVE_PHYSICS_TRIANGLE_SIMPLEX
    static_assert(sizeof(void*)==4,"native x86");AddVectoredExceptionHandler(1,fault);NativeVphysicsLibraryFixture image(argv[1]);using Function=bool(__thiscall*)(void*,const void*,float*);const auto function=reinterpret_cast<Function>(image.address(0x27710));
#else
    static_assert(sizeof(void*)==8,"owned AMD64");
#endif
    size_t cases=0;PhysicsLineSimplexInputFixture f;while(in.read(reinterpret_cast<char*>(&f),sizeof(f))){const auto before=f;
        for(size_t i=0;i<2;++i)for(float x:f.simplex.vertices[i].point)if(!std::isfinite(x))throw std::runtime_error("nonfinite simplex old fixture");for(float x:f.incoming.point)if(!std::isfinite(x))throw std::runtime_error("nonfinite simplex new fixture");
        struct Guarded {std::array<uint32_t,4>before;SourcePhysicsSimplexCandidate simplex;std::array<uint32_t,4>after;}state;state.before.fill(0xa7b8c9da);state.after=state.before;state.simplex=f.simplex;std::array<float,5>dir{123456,0,0,0,123456};std::memcpy(dir.data()+1,f.direction.data(),12);bool complete=false;
#ifdef NATIVE_PHYSICS_TRIANGLE_SIMPLEX
        active=cases;complete=function(&state.simplex,&f.incoming,dir.data()+1);
#else
        std::array<float,3>direction=f.direction;complete=sourcePhysicsTriangleSimplexCandidate(state.simplex,f.incoming,direction);std::memcpy(dir.data()+1,direction.data(),12);
#endif
        for(size_t i=0;i<4;++i)if(state.before[i]!=0xa7b8c9da||state.after[i]!=0xa7b8c9da)throw std::runtime_error("simplex state canary");if(dir.front()!=123456||dir.back()!=123456)throw std::runtime_error("simplex direction canary");if(std::memcmp(&f,&before,sizeof(f)))throw std::runtime_error("simplex input mutation");PhysicsLineSimplexOutputFixture result;result.simplex=state.simplex;std::memcpy(result.direction.data(),dir.data()+1,12);result.complete=complete;for(float x:result.direction)if(!std::isfinite(x))throw std::runtime_error("simplex direction nonfinite");out.write(reinterpret_cast<const char*>(&result),sizeof(result));++cases;
    }if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("simplex input extent");out.flush();if(!out)throw std::runtime_error("simplex output write");
#ifdef NATIVE_PHYSICS_TRIANGLE_SIMPLEX
    image.verify();
#endif
    std::cout<<"PASS triangle simplex cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
