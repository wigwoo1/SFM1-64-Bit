#include "material_matrix_scan_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#include <xmmintrin.h>
#ifdef SFM64_NATIVE_MATRIX_SCAN
#define NOMINMAX
#include <windows.h>
#include <float.h>
#include "material_construction_native_fixture.h"
static size_t activeCase=0;
static unsigned reallocations=0;
static void* __fastcall resizeScratch(void*,void*,void*p,uint32_t size){
    auto&f=*MaterialConstructionNativeFixture::active;if(size>8192)ExitProcess(98);
    size_t index=0;for(;index<f.blocks.size();++index)if(f.blocks[index].p==p&&f.blocks[index].live)break;
    if(index==f.blocks.size())ExitProcess(99);void*next=std::realloc(p,size);if(!next)ExitProcess(100);
    f.blocks[index].live=false;f.blocks.push_back({next,true});++f.releases;++f.allocations;++reallocations;return next;
}
static LONG CALLBACK referenceException(EXCEPTION_POINTERS*p){
    std::fprintf(stderr,"private_reference_case=%zu code=%08lx eip=%08lx address=%08lx\n",activeCase,p->ExceptionRecord->ExceptionCode,p->ContextRecord->Eip,p->ExceptionRecord->NumberParameters>1?static_cast<unsigned long>(p->ExceptionRecord->ExceptionInformation[1]):0ul);
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int main(int argc,char**argv){try{
    need(argc==4&&!std::ifstream(argv[3],std::ios::binary).good(),"library queries fresh-output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"Matrix scan IO");
#ifdef SFM64_NATIVE_MATRIX_SCAN
    static_assert(sizeof(void*)==4,"Reference x86");MaterialConstructionNativeFixture fixture(argv[1]);
    // Original leaf initializer only fills the CRT numeric function table.
    // No loader, DllMain, OS initialization or replacement numeric callback.
    using Init=void(__cdecl*)();reinterpret_cast<Init>(0x1007d4e7)();
    fixture.allocators[3]=reinterpret_cast<uintptr_t>(&resizeScratch);
    const auto handler=AddVectoredExceptionHandler(1,&referenceException);need(handler!=nullptr,"Private exception observer");
    unsigned old=0,control=0;_controlfp_s(&old,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
#else
    static_assert(sizeof(void*)==8,"Owned x64");
#endif
    const auto oldMxcsr=_mm_getcsr();_mm_setcsr(0x1f80);std::array<uint32_t,18>q{};size_t cases=0;
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
        need(q[0]<=1&&q[17]<=4095,"Matrix scan query");std::string text(q[17],'\0');need(bool(in.read(text.data(),text.size())),"Matrix scan framing");
        std::array<uint32_t,16>words{};std::copy(q.begin()+1,q.begin()+17,words.begin());int count=0;
#ifdef SFM64_NATIVE_MATRIX_SCAN
        activeCase=cases;
        using Scan=int(__cdecl*)(const char*,const char*,...);
        const char*format=reinterpret_cast<const char*>(q[0]?0x10095780:0x100957b4);
        auto*p=words.data();count=reinterpret_cast<Scan>(0x1007e535)(text.c_str(),format,p,p+1,p+2,p+3,p+4,p+5,p+6,p+7,p+8,p+9,p+10,p+11,p+12,p+13,p+14,p+15);
#else
        count=materialMatrixScanCandidate(text,q[0]!=0,words);
#endif
        out.write(reinterpret_cast<const char*>(&count),4);out.write(reinterpret_cast<const char*>(words.data()),64);++cases;
    }
    out.flush();need(in.eof()&&in.gcount()==0&&cases&&bool(out),"Matrix output framing");_mm_setcsr(oldMxcsr);
#ifdef SFM64_NATIVE_MATRIX_SCAN
    fixture.image.verify();need(fixture.allocations==fixture.releases&&!fixture.queues,"Scratch allocation balance");
    for(const auto&b:fixture.blocks)need(!b.live,"Live scratch block");
    std::cout<<"scratch_allocations="<<fixture.allocations<<" reallocations="<<reallocations<<'\n';_controlfp_s(&control,old,_MCW_RC|_MCW_PC);
    RemoveVectoredExceptionHandler(handler);
#endif
    std::cout<<"PASS matrix_scan_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
