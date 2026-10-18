#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "studio_decal_entry_delete_candidate.h"
static std::unordered_map<uintptr_t,uint32_t> bufferIdentities;static std::vector<uint32_t> events;static unsigned char* allocationBase;
static void check(bool v){if(!v)throw std::runtime_error("entry delete parity");}
static void put32(unsigned char* p,uint32_t v){std::memcpy(p,&v,4);}
static void put16(unsigned char* p,uint16_t v){std::memcpy(p,&v,2);}
#ifndef _WIN64
#include "studio_entry_delete_native_init.h"
static void __fastcall freeBuffer(void*,void*,void* p){events.push_back(bufferIdentities.at(reinterpret_cast<uintptr_t>(p)));}
static void __cdecl freeAllocation(void* p){events.push_back(0xf0000000u+uint32_t(static_cast<unsigned char*>(p)-allocationBase));}
#endif
int main(int argc,char**argv){try{check(argc==2);unsigned cases=0;
#ifdef _WIN64
std::ifstream file(argv[1],std::ios::binary);
#else
std::ofstream file(argv[1],std::ios::binary);auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);uint32_t table[6]={0,0,0,0,0,uint32_t(&freeBuffer)};uint32_t object=uint32_t(table),pointer=uint32_t(&object);initializeStudioEntryDeleteNative(code,uint32_t(&pointer),uint32_t(&freeAllocation));DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),code,4096);using Delete=void*(__thiscall*)(void*,uint32_t);auto destroy=reinterpret_cast<Delete>(code);
#endif
for(unsigned count=0;count<=64;++count)for(unsigned flags=0;flags<4;++flags)for(unsigned external=0;external<2;++external){events.clear();bufferIdentities.clear();unsigned size=count?count:1;std::vector<unsigned char> bytes(4+size*32,0xa5);std::vector<std::vector<unsigned char>> backing(size);for(unsigned i=0;i<size;++i){unsigned n=1+i%5;backing[i].assign(n*12,0xa5);for(unsigned j=0;j<n;++j){put16(backing[i].data()+j*12+8,j?uint16_t(j-1):uint16_t(0xffff));put16(backing[i].data()+j*12+10,j+1<n?uint16_t(j+1):uint16_t(0xffff));}bufferIdentities[reinterpret_cast<uintptr_t>(backing[i].data())]=0x1000+i;}allocationBase=bytes.data();put32(bytes.data(),count);for(unsigned i=0;i<size;++i){auto* e=bytes.data()+4+i*32;put16(e,uint16_t(i));put32(e+4,0x1000+i);put32(e+8,7);put32(e+12,external?0xffffffff:0);put16(e+16,0);put16(e+18,uint16_t(i%5));put16(e+20,0xffff);put16(e+22,uint16_t(1+i%5));put16(e+26,uint16_t(i%5));}uint32_t returned;
#ifdef _WIN64
std::vector<std::vector<EngineShadowPoolLinkCandidate>> links(size);std::vector<StudioDecalEntryCandidate> entries(size);for(unsigned i=0;i<size;++i){entries[i].chainHead=uint16_t(i);entries[i].buffer={0x1000+i,7,external?-1:0};unsigned n=1+i%5;links[i].resize(n);for(unsigned j=0;j<n;++j)links[i][j]={j?uint16_t(j-1):uint16_t(0xffff),j+1<n?uint16_t(j+1):uint16_t(0xffff)};entries[i].list.pool={links[i].data(),int32_t(n),0,uint16_t(n-1),uint16_t(n),uint16_t(n-1)};}
returned=uint32_t(studioDecalEntryDeleteCandidate(entries.data(),int32_t(count),flags,4,0,[](uintptr_t p){events.push_back(uint32_t(p));},[](uintptr_t p){events.push_back(0xf0000000u+uint32_t(p));}));for(unsigned i=0;i<size;++i){put32(bytes.data()+4+i*32+4,uint32_t(entries[i].buffer.address));put32(bytes.data()+4+i*32+8,uint32_t(entries[i].buffer.capacity));auto* e=bytes.data()+4+i*32;put16(e+16,entries[i].list.pool.head);put16(e+18,entries[i].list.pool.tail);put16(e+20,entries[i].list.freeHead);put16(e+22,entries[i].list.pool.count);for(unsigned j=0;j<links[i].size();++j){put16(backing[i].data()+j*12+8,links[i][j].previous);put16(backing[i].data()+j*12+10,links[i][j].next);}}
#else
for(unsigned i=0;i<size;++i)put32(bytes.data()+4+i*32+4,uint32_t(backing[i].data()));
returned=uint32_t(static_cast<unsigned char*>(destroy(bytes.data()+4,flags))-allocationBase);for(unsigned i=0;i<size;++i){uint32_t value;std::memcpy(&value,bytes.data()+4+i*32+4,4);if(value)put32(bytes.data()+4+i*32+4,0x1000+i);}
#endif
for(auto& b:backing)bytes.insert(bytes.end(),b.begin(),b.end());
std::vector<uint32_t> metadata{returned,uint32_t(events.size())};metadata.insert(metadata.end(),events.begin(),events.end());
#ifdef _WIN64
std::vector<uint32_t> expected(metadata.size());std::vector<unsigned char> expectedBytes(bytes.size());check(bool(file.read(reinterpret_cast<char*>(expected.data()),expected.size()*4)));check(bool(file.read(reinterpret_cast<char*>(expectedBytes.data()),expectedBytes.size())));check(metadata==expected&&bytes==expectedBytes);
#else
file.write(reinterpret_cast<char*>(metadata.data()),metadata.size()*4);file.write(reinterpret_cast<char*>(bytes.data()),bytes.size());
#endif
++cases;}
#ifdef _WIN64
check(file.peek()==std::char_traits<char>::eof());
#else
file.flush();check(bool(file));VirtualFree(code,0,MEM_RELEASE);
#endif
std::cout<<"entry_delete_native_cases="<<cases<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
