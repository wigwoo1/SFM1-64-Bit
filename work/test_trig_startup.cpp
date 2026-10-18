#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "trig_startup_native_init.h"
#include "studio_trigonometric_table_candidate.h"
static void check(bool b){if(!b)std::abort();}
static std::array<double,256> angles;static unsigned calls;
static double __vectorcall controlledSine(double angle){check(calls<256);angles[calls++]=angle;return std::sin(angle);}
struct Ops {double sine(double angle){return controlledSine(angle);}};
int main(){std::array<float,256> values{};std::array<double,3> constants{trigonometricConstants[0],trigonometricConstants[1],trigonometricConstants[2]};
#if !defined(_WIN64)
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);const auto& body=trigStartupBodies[0];std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);dll.seekg(body.fileOffset);dll.read(reinterpret_cast<char*>(code),body.size);check(bool(dll)&&!std::memcmp(code,body.bytes,body.size));for(unsigned i=0;i<body.fixupCount;++i){auto fix=body.fixups[i];uintptr_t p=0;switch(fix[1]){case 0x100fc8c8:p=reinterpret_cast<uintptr_t>(&constants[0]);break;case 0x100fc8c0:p=reinterpret_cast<uintptr_t>(&constants[1]);break;case 0x10152468:p=reinterpret_cast<uintptr_t>(&constants[2]);break;case 0x101986e0:p=reinterpret_cast<uintptr_t>(values.data());break;default:std::abort();}unsigned value=static_cast<unsigned>(p);std::memcpy(code+fix[0],&value,4);}check(body.callCount==1&&body.calls[0][1]==0x100e529b);unsigned offset=body.calls[0][0];unsigned delta=static_cast<unsigned>(reinterpret_cast<uintptr_t>(&controlledSine)-reinterpret_cast<uintptr_t>(code+offset+5));std::memcpy(code+offset+1,&delta,4);DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old)&&FlushInstructionCache(GetCurrentProcess(),code,4096));reinterpret_cast<void(__cdecl*)()>(code)();
#else
 check(reinterpret_cast<uintptr_t>(values.data())>UINT32_MAX);Ops ops;studioInitializeTrigonometricTableCandidate(values,constants,ops);
#endif
 check(calls==256);for(unsigned i=0;i<256;++i){double angle=static_cast<double>(i);angle*=2;angle*=constants[1];angle*=1.0/256;check(angles[i]==angle&&values[i]==static_cast<float>(std::sin(angle)));}std::array<unsigned,768> wire{};for(unsigned i=0;i<256;++i){std::memcpy(&wire[3*i],&angles[i],8);std::memcpy(&wire[3*i+2],&values[i],4);}
#if !defined(_WIN64)
 std::ofstream output("outputs/native-trig-startup.bin",std::ios::binary|std::ios::trunc);output.write(reinterpret_cast<char*>(wire.data()),sizeof(wire));check(bool(output));check(VirtualFree(code,0,MEM_RELEASE));
#else
 std::ifstream input("outputs/native-trig-startup.bin",std::ios::binary);std::array<unsigned,768> original{};input.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(input)&&wire==original&&input.peek()==std::char_traits<char>::eof());
#endif
 std::printf("trigonometric_startup_%zu_bit_entries=256 passed\n",sizeof(void*)*8);
}
