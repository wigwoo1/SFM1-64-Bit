#define NOMINMAX
#include <Windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "studio_math_library_startup_candidate.h"
#if !defined(_WIN64)
#include "math_startup_native_init.h"
#endif
static void check(bool b){if(!b)std::abort();}
static StudioMathLibraryStartupCandidate state;static std::array<unsigned char,8> cpu;static std::vector<unsigned> events;static bool mutate;static float expectedGamma,expectedTexture,expectedBrightness;static int expectedOverbright;
static void* __cdecl cpuInfo(){events.push_back(1);return cpu.data();}
static void __cdecl diagnostic(const char* message,...){check(!std::strcmp(message,"SSE and SSE2 are required."));events.push_back(2);state.initialized=true;}
static void __cdecl trig(){check(state.initialized);events.push_back(3);if(mutate)state.initialized=false;}
static void __cdecl gamma(float a,float b,float c,int d){check(a==expectedGamma&&b==expectedTexture&&c==expectedBrightness&&d==expectedOverbright);events.push_back(4);}
static void __cdecl seed(unsigned s){check(s==0x31415926);events.push_back(5);}
struct Ops {unsigned cpuFlags(){cpuInfo();return cpu[4];}void diagnostic(const char* m){::diagnostic(m);}void initializeTrigonometricTable(){trig();}void buildGammaTables(float a,float b,float c,int d){gamma(a,b,c,d);}void seedGenerator(unsigned s){seed(s);}};
int main(){unsigned cases=0;
#if !defined(_WIN64)
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);auto cpuCallback=&cpuInfo;auto diagnosticCallback=&diagnostic;const auto& body=mathStartupBodies[0];std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);dll.seekg(body.fileOffset);dll.read(reinterpret_cast<char*>(code),body.size);check(bool(dll)&&!std::memcmp(code,body.bytes,body.size));for(unsigned i=0;i<body.fixupCount;++i){auto fix=body.fixups[i];uintptr_t p=0;switch(fix[1]){case 0x10198ae0:p=reinterpret_cast<uintptr_t>(&state.initialized);break;case 0x100fa1fc:p=reinterpret_cast<uintptr_t>(&cpuCallback);break;case 0x1015247c:p=reinterpret_cast<uintptr_t>(mathStartupDiagnostic);break;case 0x100fa23c:p=reinterpret_cast<uintptr_t>(&diagnosticCallback);break;default:std::abort();}unsigned value=static_cast<unsigned>(p);std::memcpy(code+fix[0],&value,4);}for(unsigned i=0;i<body.callCount;++i){auto call=body.calls[i];uintptr_t p=0;switch(call[1]){case 0x100d5440:p=reinterpret_cast<uintptr_t>(&trig);break;case 0x100d5020:p=reinterpret_cast<uintptr_t>(&gamma);break;case 0x100d5eb0:p=reinterpret_cast<uintptr_t>(&seed);break;default:std::abort();}unsigned delta=static_cast<unsigned>(p-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old)&&FlushInstructionCache(GetCurrentProcess(),code,4096));auto init=reinterpret_cast<void(__cdecl*)(float,float,float,int,int,int,int,int)>(code);std::ofstream output("outputs/native-math-startup.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream input("outputs/native-math-startup.bin",std::ios::binary);Ops ops;
#endif
 for(bool initialized:{false,true})for(unsigned flags:{0u,8u,16u,24u})for(bool profile:{false,true})for(bool mutation:{false,true}){state.initialized=initialized;cpu.fill(0xab);cpu[4]=static_cast<unsigned char>(flags);mutate=mutation;expectedGamma=profile?1.5f:2.2f;expectedTexture=profile?1.6f:2.2f;expectedBrightness=profile?0.1f:0;expectedOverbright=profile?1:2;events.clear();
#if !defined(_WIN64)
 init(expectedGamma,expectedTexture,expectedBrightness,expectedOverbright,0,0,0,0);
#else
 studioMathLibraryInitializeCandidate(state,expectedGamma,expectedTexture,expectedBrightness,expectedOverbright,ops);
#endif
 std::vector<unsigned> expected;if(!initialized){expected.push_back(1);if(flags!=24)expected.push_back(2);expected.insert(expected.end(),{3,4,5});}check(events==expected&&state.initialized==(initialized||!mutation));for(unsigned i=0;i<8;++i)check(cpu[i]==(i==4?flags:0xab));std::array<unsigned,8> wire{};wire[0]=static_cast<unsigned>(events.size());std::copy(events.begin(),events.end(),wire.begin()+1);wire[7]=unsigned(state.initialized);
#if !defined(_WIN64)
 output.write(reinterpret_cast<char*>(wire.data()),sizeof(wire));check(bool(output));
#else
 std::array<unsigned,8> original{};input.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(input)&&wire==original);
#endif
 ++cases;}
#if !defined(_WIN64)
 check(VirtualFree(code,0,MEM_RELEASE));
#else
 check(input.peek()==std::char_traits<char>::eof());
#endif
 std::printf("math_library_startup_%zu_bit_cases=%u passed\n",sizeof(void*)*8,cases);
}
