#define NOMINMAX
#include <windows.h>
#include <fstream>
#include <vector>
#include <iostream>
#include <cstring>
#include <stdexcept>
int main(int argc,char**argv){char*mapped=nullptr;try{static_assert(sizeof(void*)==4,"x86 required");if(argc!=2||std::ifstream(argv[1]).good())throw std::runtime_error("fresh oracle output required");std::ifstream file("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\sdktools\\python\\2.7\\win32\\Lib\\site-packages\\vs\\valve_pyd.pyd",std::ios::binary);std::vector<char>bytes((std::istreambuf_iterator<char>(file)),{});if(bytes.size()<4096)throw std::runtime_error("DLL missing");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(bytes.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("image base");mapped=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(mapped!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred mapping unavailable");auto sections=IMAGE_FIRST_SECTION(nt);for(unsigned n=0;n<nt->FileHeader.NumberOfSections;++n){if(size_t(sections[n].PointerToRawData)+sections[n].SizeOfRawData>bytes.size())throw std::runtime_error("section bounds");std::memcpy(mapped+sections[n].VirtualAddress,bytes.data()+sections[n].PointerToRawData,sections[n].SizeOfRawData);}const unsigned char instructions[]={0x66,0x0f,0x6e,0x02,0x0f,0x5b,0xc0,0xf3,0x0f,0x59,0x05,0x78,0x3c,0x35,0x10};if(std::memcmp(mapped+0x15fb64,instructions,15))throw std::runtime_error("native instruction fingerprint mismatch");reinterpret_cast<unsigned char*>(mapped)[0x15fb73]=0xc3;FlushInstructionCache(GetCurrentProcess(),mapped,nt->OptionalHeader.SizeOfImage);std::ofstream output(argv[1],std::ios::binary);size_t cases=0;auto native=mapped+0x15fb64;
 for(int32_t tick:{INT32_MIN,INT32_MIN+1,-10000001,-10001,-10000,-9999,-1,0,1,9999,10000,10001,870833,880833,16777215,16777216,16777217,INT32_MAX}){float seconds=0;__asm {
 lea edx,tick
 mov eax,native
 call eax
 movss seconds,xmm0
 }output.write(reinterpret_cast<const char*>(&tick),4);output.write(reinterpret_cast<const char*>(&seconds),4);++cases;}output.close();if(!output)throw std::runtime_error("oracle write");VirtualFree(mapped,0,MEM_RELEASE);mapped=nullptr;std::cout<<"native tick seconds oracle passed cases="<<cases<<" isolated_original_instructions=1 DLL_entrypoint_called=0\n";return 0;
 }catch(const std::exception&e){if(mapped)VirtualFree(mapped,0,MEM_RELEASE);std::cerr<<e.what()<<'\n';return 1;}}
