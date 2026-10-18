#include "physics_refinement_face_test_fixture.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#ifdef NATIVE_PHYSICS_REFINEMENT_FACE
#define NOMINMAX
#include <windows.h>
#include "native_vphysics_library_fixture.h"
static size_t active=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"refinement face fault case="<<active<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("library directory input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("refinement face fixture IO");
#ifdef NATIVE_PHYSICS_REFINEMENT_FACE
    static_assert(sizeof(void*)==4,"native x86");AddVectoredExceptionHandler(1,fault);NativeVphysicsLibraryFixture image(argv[1]);using Function=float*(__thiscall*)(void*,float*,const float*);
#else
    static_assert(sizeof(void*)==8,"owned AMD64");
#endif
    size_t cases=0;PhysicsRefinementFaceInputFixture f;while(in.read(reinterpret_cast<char*>(&f),sizeof(f))){const auto before=f;if(f.includeBase>1)throw std::runtime_error("refinement face mode");for(const auto&v:f.simplex.vertices)for(float x:v.point)if(!std::isfinite(x))throw std::runtime_error("nonfinite refinement point fixture");for(float x:f.direction)if(!std::isfinite(x))throw std::runtime_error("nonfinite refinement direction fixture");
        struct Guarded{std::array<uint32_t,4>before;SourcePhysicsSimplexCandidate simplex;std::array<uint32_t,4>after;}g;g.before.fill(0xa7b8c9da);g.after=g.before;g.simplex=f.simplex;std::array<float,5>normal{123456,0,0,0,123456};std::memcpy(normal.data()+1,f.initialOutput.data(),12);
#ifdef NATIVE_PHYSICS_REFINEMENT_FACE
        active=cases;const auto function=reinterpret_cast<Function>(image.address(f.includeBase?0x25460:0x25b10));if(function(&g.simplex,normal.data()+1,f.direction.data())!=normal.data()+1)throw std::runtime_error("original refinement return pointer");
#else
        const auto answer=sourcePhysicsRefinementFaceCandidate(g.simplex,f.direction,f.includeBase!=0);std::memcpy(normal.data()+1,answer.data(),12);
#endif
        if(normal.front()!=123456||normal.back()!=123456)throw std::runtime_error("refinement output canary");for(unsigned j=0;j<4;++j)if(g.before[j]!=0xa7b8c9da||g.after[j]!=0xa7b8c9da)throw std::runtime_error("refinement simplex canary");if(std::memcmp(&f,&before,sizeof(f)))throw std::runtime_error("refinement input mutation");PhysicsRefinementFaceOutputFixture result;result.simplex=g.simplex;std::memcpy(result.normal.data(),normal.data()+1,12);out.write(reinterpret_cast<const char*>(&result),sizeof(result));++cases;
    }if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("refinement fixture extent");out.flush();if(!out)throw std::runtime_error("refinement output write");
#ifdef NATIVE_PHYSICS_REFINEMENT_FACE
    image.verify();
#endif
    std::cout<<"PASS refinement-face cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
