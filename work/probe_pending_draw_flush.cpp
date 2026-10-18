#include <array>
#include <windows.h>
#include "pending_draw_flush_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>
#include <algorithm>
#ifndef _WIN64
static uint8_t* activeState;static uint32_t events[2];static unsigned eventCount;
static void nativeEvent(unsigned id){if(eventCount>=2)throw std::runtime_error("unexpected reentry");events[eventCount++]=id|(uint32_t(activeState[0x30])<<8)|(uint32_t(activeState[0x32])<<16)|(uint32_t(activeState[0x33])<<24);}
static void reenter(){reinterpret_cast<void(__thiscall*)(void*)>(0x10007f50)(activeState);}
static void __fastcall ordinary(void*,void*,int32_t first,int32_t second){if(first!=-1||second)throw std::runtime_error("ordinary args");nativeEvent(1);reenter();}
static void __fastcall alternate(void*,void*,void* data,int32_t first,int32_t second){if(data!=activeState+0x1c||first!=-1||second)throw std::runtime_error("alternate args");nativeEvent(2);reenter();}
static void __fastcall finish(void*,void*,int32_t first,int32_t second){if(first||second)throw std::runtime_error("finish args");nativeEvent(3);}
#endif
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("oracle path required");
#ifdef _WIN64
 std::ifstream input(argv[1],std::ios::binary);if(!input)throw std::runtime_error("oracle open");
 unsigned cases=0;std::array<uint32_t,10> record;
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  PendingDrawFlushCandidate state{record[0]!=0,uint8_t(record[1]),uint8_t(record[2]),uint8_t(record[3])};std::array<uint32_t,2> events{};unsigned eventCount=0;
  auto event=[&](unsigned id){if(eventCount>=2)throw std::runtime_error("owned reentry");events[eventCount++]=id|(uint32_t(state.guard)<<8)|(uint32_t(state.pending)<<16)|(uint32_t(state.alternate)<<24);};
  state.flush([&](bool alternate){event(alternate?2:1);unsigned nested=0;state.flush([&](bool){++nested;},[&]{++nested;});if(nested)throw std::runtime_error("reentry callback");},[&]{event(3);});
  if(state.guard!=record[4]||state.pending!=record[5]||state.alternate!=record[6]||eventCount!=record[7]||events[0]!=record[8]||events[1]!=record[9])throw std::runtime_error("flush mismatch");++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 std::cout<<"x64_native_pending_flushes_exact="<<cases<<'\n';
#else
 if(std::ifstream(argv[1]).good())throw std::runtime_error("fresh oracle required");
 std::ifstream input(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 std::vector<uint8_t> file((std::istreambuf_iterator<char>(input)),{});
 if(file.size()<sizeof(IMAGE_DOS_HEADER))throw std::runtime_error("DOS bounds");
 auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());
 if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||size_t(dos->e_lfanew)>file.size()-sizeof(IMAGE_NT_HEADERS32))throw std::runtime_error("PE bounds");
 auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);
 if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC||nt->OptionalHeader.ImageBase!=0x10000000||nt->OptionalHeader.SizeOfHeaders>file.size())throw std::runtime_error("PE identity");
 auto allocation=VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 if(!allocation)throw std::runtime_error("private preferred-base mapping unavailable");
 std::unique_ptr<void,void(*)(void*)> owner(allocation,[](void* p){VirtualFree(p,0,MEM_RELEASE);});
 std::memcpy(allocation,file.data(),nt->OptionalHeader.SizeOfHeaders);
 auto sections=IMAGE_FIRST_SECTION(nt);
 if(reinterpret_cast<const uint8_t*>(sections)+size_t(nt->FileHeader.NumberOfSections)*sizeof(IMAGE_SECTION_HEADER)>file.data()+file.size())throw std::runtime_error("section table bounds");
 for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){auto& section=sections[i];
  if(section.PointerToRawData>file.size()||section.SizeOfRawData>file.size()-section.PointerToRawData||section.VirtualAddress>nt->OptionalHeader.SizeOfImage||section.SizeOfRawData>nt->OptionalHeader.SizeOfImage-section.VirtualAddress)throw std::runtime_error("section bounds");
  std::memcpy(static_cast<uint8_t*>(allocation)+section.VirtualAddress,file.data()+section.PointerToRawData,section.SizeOfRawData);
 }
 DWORD oldProtect;if(!VirtualProtect(allocation,nt->OptionalHeader.SizeOfImage,PAGE_EXECUTE_READ,&oldProtect))throw std::runtime_error("code protection");
 FlushInstructionCache(GetCurrentProcess(),allocation,nt->OptionalHeader.SizeOfImage);
 using Flush=void(__thiscall*)(void*);auto flush=reinterpret_cast<Flush>(0x10007f50);
 std::array<void*,26> table{};table[12]=reinterpret_cast<void*>(&ordinary);table[25]=reinterpret_cast<void*>(&alternate);table[22]=reinterpret_cast<void*>(&finish);auto tp=table.data();void* endpoint=&tp;
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,10> record{};record[0]=i&1;record[1]=(i>>1)&1;record[2]=(i>>2)&255;record[3]=(i>>10)&63;
  std::array<uint8_t,0x40> object{};auto ep=record[0]?endpoint:nullptr;std::memcpy(object.data()+0x18,&ep,4);object[0x30]=uint8_t(record[1]);object[0x32]=uint8_t(record[2]);object[0x33]=uint8_t(record[3]);activeState=object.data();eventCount=0;events[0]=events[1]=0;
  flush(object.data());record[4]=object[0x30];record[5]=object[0x32];record[6]=object[0x33];record[7]=eventCount;record[8]=events[0];record[9]=events[1];
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_pending_flushes=65536\n";
#endif
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
