#define NOMINMAX
#include <Windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "base_startup_native_init.h"
static void check(bool b){if(!b)std::abort();}
using Factory=uintptr_t(__cdecl*)(const char*,int*);
static uintptr_t incoming,cvarProvider,fileProvider,utility,manager,accessObject;
static unsigned accessFlags;static bool cvarPresent,filePresent,externalPresent,localPresent;
static std::vector<unsigned> trace;
static uintptr_t __cdecl external(const char* name,int* status){check(status==nullptr&&!std::strcmp(name,"VShaderUtil001"));trace.push_back(5);return externalPresent?0x1234:0;}
static uintptr_t __cdecl local(const char* name,int* status){check(status==nullptr&&!std::strcmp(name,"VShaderUtil001"));trace.push_back(6);return localPresent?0x1234:0;}
static Factory __cdecl localGetter(){return &local;}
static uintptr_t __cdecl wrapper(const char*,int*){std::abort();}
static void __cdecl connector(Factory* factories,int count){check(count==1&&factories[0]==&wrapper&&incoming==reinterpret_cast<uintptr_t>(&external));cvarProvider=cvarPresent?0x1234:0;fileProvider=filePresent?0x1234:0;trace.push_back(1);}
static void __cdecl registerCvar(unsigned flags,void* object){check(flags==0x800000&&object==&accessObject&&cvarProvider&&accessFlags==3);trace.push_back(3);}
static void __cdecl resources(Factory* factories,int count){check(count==1&&factories[0]==&wrapper&&incoming==reinterpret_cast<uintptr_t>(&external));trace.push_back(4);}
static void __cdecl warning(const char* text,...){check(incoming==0&&!std::strcmp(text,"ShaderAPIDx10 was unable to access the required interfaces!\n"));trace.push_back(7);}
static void __cdecl timing(float a,float b,float c,int d,int e,int f,int g,int h){check(incoming==0&&a==2.2f&&b==2.2f&&c==0&&d==2&&e==1&&f==1&&g==1&&h==1);trace.push_back(8);}
int main(){static_assert(sizeof(void*)==4,"originalx86");auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);auto warningCallback=&warning;float value=2.2f;std::array<unsigned,2> receiver{};unsigned fakeVtable=0;const auto& body=baseStartupBodies[0];std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);dll.seekg(body.fileOffset);dll.read(reinterpret_cast<char*>(code),body.size);check(bool(dll)&&!std::memcmp(code,body.bytes,body.size));
 for(unsigned i=0;i<body.fixupCount;++i){auto fix=body.fixups[i];uintptr_t p=0;switch(fix[1]){case 0x10183b48:p=reinterpret_cast<uintptr_t>(&incoming);break;case 0x10027c00:p=reinterpret_cast<uintptr_t>(&wrapper);break;case 0x10183b88:p=reinterpret_cast<uintptr_t>(&accessFlags);break;case 0x10183b84:p=reinterpret_cast<uintptr_t>(&accessObject);break;case 0x100fdb2c:p=reinterpret_cast<uintptr_t>(&fakeVtable);break;case 0x101908e4:p=reinterpret_cast<uintptr_t>(&cvarProvider);break;case 0x100fdb74:p=reinterpret_cast<uintptr_t>("VShaderUtil001");break;case 0x10183b58:p=reinterpret_cast<uintptr_t>(&utility);break;case 0x10183b54:p=reinterpret_cast<uintptr_t>(&manager);break;case 0x1019090c:p=reinterpret_cast<uintptr_t>(&fileProvider);break;case 0x100fdb70:p=reinterpret_cast<uintptr_t>(&value);break;case 0x100fdb30:p=reinterpret_cast<uintptr_t>("ShaderAPIDx10 was unable to access the required interfaces!\n");break;case 0x100fa244:p=reinterpret_cast<uintptr_t>(&warningCallback);break;default:std::abort();}unsigned address=static_cast<unsigned>(p);std::memcpy(code+fix[0],&address,4);}
 for(unsigned i=0;i<body.callCount;++i){auto call=body.calls[i];uintptr_t p=0;switch(call[1]){case 0x100dbe10:p=reinterpret_cast<uintptr_t>(&connector);break;case 0x100d6070:p=reinterpret_cast<uintptr_t>(&registerCvar);break;case 0x100ddf80:p=reinterpret_cast<uintptr_t>(&resources);break;case 0x100d6060:p=reinterpret_cast<uintptr_t>(&localGetter);break;case 0x100d5520:p=reinterpret_cast<uintptr_t>(&timing);break;default:std::abort();}unsigned delta=static_cast<unsigned>(p-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}
 DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old)&&FlushInstructionCache(GetCurrentProcess(),code,4096));auto connect=reinterpret_cast<bool(__thiscall*)(void*,Factory)>(code);std::ofstream output("outputs/native-base-startup.bin",std::ios::binary|std::ios::trunc);unsigned cases=0;
 for(bool cvar:{false,true})for(bool file:{false,true})for(bool ext:{false,true})for(bool loc:{false,true})for(bool self:{false,true})for(bool initialized:{false,true}){cvarPresent=cvar;filePresent=file;externalPresent=ext;localPresent=loc;incoming=utility=manager=0xabababab;accessFlags=initialized?3:2;accessObject=0x1111;receiver.fill(0xabababab);trace.clear();bool result=connect(self?receiver.data():nullptr,&external);bool success=file&&self&&(ext||loc);std::vector<unsigned> expected{1};if(cvar)expected.push_back(3);expected.push_back(4);expected.push_back(5);if(!ext)expected.push_back(6);expected.push_back(success?8:7);check(result==success&&trace==expected&&incoming==0&&utility==((ext||loc)?0x1234u:0u)&&manager==(self?reinterpret_cast<uintptr_t>(receiver.data()):0)&&accessFlags==3&&accessObject==(initialized?0x1111:reinterpret_cast<uintptr_t>(&fakeVtable))&&receiver[0]==0xabababab&&receiver[1]==0xabababab);std::array<unsigned,16> wire{};wire[0]=static_cast<unsigned>(trace.size());std::copy(trace.begin(),trace.end(),wire.begin()+1);wire[10]=utility?1:0;wire[11]=manager?1:0;wire[12]=accessFlags;wire[13]=static_cast<unsigned>(incoming);wire[14]=unsigned(result);wire[15]=initialized?1:2;output.write(reinterpret_cast<char*>(wire.data()),sizeof(wire));check(bool(output));++cases;}
 check(VirtualFree(code,0,MEM_RELEASE));std::printf("original_base_startup_cases=%u passed\n",cases);
}
