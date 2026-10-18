#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "studio_material_queue_drain_candidate.h"
static std::vector<uint32_t> events;static unsigned length,mode,resets;
static void check(bool v){if(!v)throw std::runtime_error("drain parity");}
#ifndef _WIN64
#include "material_queue_drain_native_init.h"
static uint32_t queue[10];static std::vector<uint32_t> nodes,calls;
static void append(){if(length){nodes[(length-1)*2]=uint32_t(nodes.data()+length*2);queue[1]=uint32_t(nodes.data()+length*2);}}
static void __fastcall invoke(void* receiver,void*){auto* call=static_cast<uint32_t*>(receiver);events.push_back(call[1]*2);if(call[1]==0&&mode==1)append();}
static void __fastcall release(void* receiver,void*){auto* call=static_cast<uint32_t*>(receiver);events.push_back(call[1]*2+1);if(call[1]==0&&mode==2)append();}
static void __fastcall reset(void* receiver,void*,uint32_t flag){check(receiver==queue+2&&!flag&&queue[0]&&queue[1]);++resets;}
#endif
int main(int argc,char**argv){try{check(argc==2);unsigned cases=0;
#ifdef _WIN64
std::ifstream file(argv[1],std::ios::binary);
#else
std::ofstream file(argv[1],std::ios::binary);auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);initializeMaterialQueueDrainNative(code,uint32_t(&reset));DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),code,4096);using Drain=void(__thiscall*)(void*);auto drain=reinterpret_cast<Drain>(code);uint32_t table[3]={0,uint32_t(&release),uint32_t(&invoke)};
#endif
for(length=0;length<=128;++length)for(mode=0;mode<3;++mode){events.clear();resets=0;std::vector<uint32_t> result;
#ifdef _WIN64
std::vector<StudioMaterialQueueNodeCandidate> nodes(length+1);std::vector<StudioDecalQueuedCallCandidate> calls(length+1);StudioMaterialQueueCandidate queue;for(unsigned i=0;i<=length;++i){calls[i].argument=i;nodes[i].call=&calls[i];if(i<length)studioMaterialQueueAppendCandidate(queue,&nodes[i]);}auto append=[&]{if(length)studioMaterialQueueAppendCandidate(queue,&nodes[length]);};studioMaterialQueueDrainCandidate(queue,[&](auto* call){events.push_back(uint32_t(call->argument)*2);if(!call->argument&&mode==1)append();},[&](auto* call){events.push_back(uint32_t(call->argument)*2+1);if(!call->argument&&mode==2)append();},[&](bool flag){check(!flag&&queue.head&&queue.tail);++resets;});result={resets,uint32_t(!queue.head&&!queue.tail)};for(auto& node:nodes)result.push_back(node.next?uint32_t(node.next-nodes.data()):0xffffffffu);
#else
std::memset(queue,0xa5,sizeof(queue));queue[0]=0;queue[1]=0;nodes.assign((length+1)*2,0);calls.assign((length+1)*2,0);for(unsigned i=0;i<=length;++i){calls[i*2]=uint32_t(table);calls[i*2+1]=i;nodes[i*2]=i+1<length?uint32_t(nodes.data()+(i+1)*2):0;nodes[i*2+1]=uint32_t(calls.data()+i*2);}if(length){queue[0]=uint32_t(nodes.data());queue[1]=uint32_t(nodes.data()+(length-1)*2);}drain(queue);for(unsigned i=2;i<10;++i)check(queue[i]==0xa5a5a5a5);result={resets,uint32_t(!queue[0]&&!queue[1])};for(unsigned i=0;i<=length;++i)result.push_back(nodes[i*2]?(nodes[i*2]-uint32_t(nodes.data()))/8:0xffffffffu);
#endif
result.push_back(uint32_t(events.size()));result.insert(result.end(),events.begin(),events.end());
#ifdef _WIN64
std::vector<uint32_t> expected(result.size());check(bool(file.read(reinterpret_cast<char*>(expected.data()),expected.size()*4)));check(result==expected);
#else
file.write(reinterpret_cast<char*>(result.data()),result.size()*4);
#endif
++cases;}
#ifdef _WIN64
check(file.peek()==std::char_traits<char>::eof());
#else
file.flush();check(bool(file));VirtualFree(code,0,MEM_RELEASE);
#endif
std::cout<<"material_drain_cases="<<cases<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
