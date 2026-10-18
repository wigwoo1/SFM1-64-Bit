#define NOMINMAX
#include <windows.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "engine_projection_receiver_gate_candidate.h"
#include "engine_projection_generate_candidate.h"
static uint32_t observed[4];static unsigned outsideCase;static unsigned char rawRecord[24];
#ifndef _WIN64
#include "engine_projection_receiver_gate_native_init.h"
void* __fastcall readRecord(void*,void*){++observed[0];return rawRecord;}
void __fastcall readBounds(void*,void*,float* low,float* high){++observed[1];for(int k=0;k<3;++k){low[k]=outsideCase?3.f:.25f;high[k]=outsideCase?4.f:.75f;}}
void __fastcall associate(void*,void*,void* record,uint32_t handle){if(record!=rawRecord)throw std::runtime_error("association record");++observed[2];observed[3]=handle;}
#endif
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("oracle path");float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},center[3]={.5f,.5f,.5f};EngineProjectionPlanesCandidate planes;engineProjectionGeneratePlanesCandidate(identity,planes);
#ifdef _WIN64
 std::ifstream oracle(argv[1],std::ios::binary);
#else
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,projectionReceiverGateAllocationSize,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("allocate");initializeProjectionReceiverGateNative(code,reinterpret_cast<uint32_t>(&associate));DWORD old;if(!VirtualProtect(code,projectionReceiverGateAllocationSize,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),code,projectionReceiverGateAllocationSize);
 using Gate=void(__thiscall*)(void*,void*,void*,uint32_t);auto gate=reinterpret_cast<Gate>(code);std::ofstream oracle(argv[1],std::ios::binary);
 uint32_t objectTable[6]{};objectTable[3]=reinterpret_cast<uint32_t>(&readBounds);objectTable[5]=reinterpret_cast<uint32_t>(&readRecord);uint32_t object=reinterpret_cast<uint32_t>(objectTable);
 uint32_t ownerTable[8]{};ownerTable[7]=reinterpret_cast<uint32_t>(code+projectionReceiverGateAccessorOffset);std::vector<unsigned char> owner(0x141a8),shadows(0x2a0);alignas(16) unsigned char projections[0x6c0]{};
 auto put=[](unsigned char* p,uint32_t v){std::memcpy(p,&v,4);};put(owner.data(),reinterpret_cast<uint32_t>(ownerTable));put(owner.data()+0x1c,reinterpret_cast<uint32_t>(shadows.data()));put(owner.data()+0x141a4,reinterpret_cast<uint32_t>(projections));uint16_t index=1;std::memcpy(shadows.data()+0x150+0xdc,&index,2);std::memcpy(projections+0x360,&planes,320);
 unsigned char payload[48]{};uint16_t handle=1;std::memcpy(payload,&handle,2);std::memcpy(payload+28,center,12);float radius=1;std::memcpy(payload+40,&radius,4);
#endif
 unsigned cases=0;for(unsigned special=0;special<2;++special)for(unsigned mask=0;mask<8;++mask)for(unsigned eligible=0;eligible<2;++eligible)for(unsigned shadow=0;shadow<8;++shadow)for(unsigned outside=0;outside<2;++outside){
 uint32_t flags=(mask&1?0x20:0)|(mask&2?0x1000:0)|(mask&4?0x2000:0);std::memset(observed,0,sizeof(observed));outsideCase=outside;
#ifdef _WIN64
 EngineProjectionObjectRecordCandidate record{flags,uint8_t(eligible)};
 engineProjectionReceiverGateCandidate(special?0x101:0x100,1,center,1,[&]{++observed[0];return &record;},[&](float* low,float* high){++observed[1];for(int k=0;k<3;++k){low[k]=outside?3.f:.25f;high[k]=outside?4.f:.75f;}},[&](uint16_t)->const EngineProjectionPlanesCandidate&{return planes;},[&](uint16_t){return shadow;},[&](auto*,uint16_t h){++observed[2];observed[3]=h;});uint32_t expected[4];if(!oracle.read(reinterpret_cast<char*>(expected),16)||std::memcmp(expected,observed,16))throw std::runtime_error("native gate mismatch");
#else
 std::memset(rawRecord,0,24);std::memcpy(rawRecord,&flags,4);rawRecord[0x16]=uint8_t(eligible);shadows[0x150+0x78]=uint8_t(shadow);gate(owner.data(),payload,&object,special?0x101:0x100);oracle.write(reinterpret_cast<char*>(observed),16);
#endif
 ++cases;}
#ifdef _WIN64
 if(oracle.peek()!=std::char_traits<char>::eof())throw std::runtime_error("extra oracle");
#else
 oracle.flush();if(!oracle)throw std::runtime_error("write");VirtualFree(code,0,MEM_RELEASE);
#endif
 std::cout<<"gate_cases="<<cases<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
