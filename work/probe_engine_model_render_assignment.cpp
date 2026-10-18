#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "model_render_record_assignment_candidate.h"
#ifndef _WIN64
#include "engine_model_render_assignment_native_bytes.h"
#endif
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("oracle path");
#ifdef _WIN64
std::ifstream f(argv[1],std::ios::binary);
#else
std::ofstream f(argv[1],std::ios::binary);auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,sizeof(modelRenderAssignmentNativeBytes),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("allocate");std::memcpy(code,modelRenderAssignmentNativeBytes,sizeof(modelRenderAssignmentNativeBytes));DWORD old;if(!VirtualProtect(code,sizeof(modelRenderAssignmentNativeBytes),PAGE_EXECUTE_READ,&old))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),code,sizeof(modelRenderAssignmentNativeBytes));using Assign=void(__thiscall*)(void*,uint32_t,const uint32_t*);auto assign=reinterpret_cast<Assign>(code);
#endif
unsigned cases=0;for(unsigned flags=0;flags<65536;++flags)for(unsigned mode=0;mode<4;++mode){uint32_t value=flags*65537u,argument=(mode&2)?0xabcdffffu:0xabcd0000u;uint32_t result[2]={};
#ifdef _WIN64
ModelRenderRecordAssignmentCandidate record{uint16_t(flags),0xa5a5a5a5};modelRenderRecordAssignmentCandidate(argument,mode&1?&value:nullptr,[&](uint16_t index)->ModelRenderRecordAssignmentCandidate&{if(index)throw std::runtime_error("index");return record;});result[0]=record.flags;result[1]=record.value;uint32_t expected[2];if(!f.read(reinterpret_cast<char*>(expected),8)||std::memcmp(result,expected,8))throw std::runtime_error("assignment mismatch");
#else
unsigned char record[264];std::memset(record,0xa5,264);uint16_t initial=uint16_t(flags);std::memcpy(record+250,&initial,2);uint32_t owner[4]={0,0,0,uint32_t(reinterpret_cast<uintptr_t>(record))};assign(owner,argument,mode&1?&value:nullptr);uint16_t final;std::memcpy(&final,record+250,2);result[0]=final;std::memcpy(result+1,record+252,4);for(unsigned i=0;i<264;++i)if((i<250||i>=256)&&record[i]!=0xa5)throw std::runtime_error("payload changed");f.write(reinterpret_cast<char*>(result),8);
#endif
++cases;}
#ifdef _WIN64
if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("extra oracle");
#else
f.flush();if(!f)throw std::runtime_error("write");VirtualFree(code,0,MEM_RELEASE);
#endif
std::cout<<"assignment_cases="<<cases<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
