#pragma once
#include "engine_projection_chain_native_bytes.h"
inline void initializeProjectionChainNative(unsigned char* code){
std::memcpy(code+0,generateChainBytes,567);
std::memcpy(code+576,planeChainBytes,907);
std::memcpy(code+1488,normalizeChainBytes,109);
std::memcpy(code+1600,installChainBytes,31);
std::memcpy(code+1632,fourChainBytes,119);
std::memcpy(code+1760,twoChainBytes,125);
std::memcpy(code+1888,derivedChainBytes,114);
{uint32_t values[4]={1065353216u,1065353216u,1065353216u,1065353216u};std::memcpy(code+2032,values,16);}
{uint32_t values[4]={872415232u,872415232u,872415232u,872415232u};std::memcpy(code+2048,values,16);}
{uint32_t values[4]={3141592654u,3141592654u,3141592654u,3141592654u};std::memcpy(code+2064,values,16);}
{uint32_t values[4]={0u,0u,0u,0u};std::memcpy(code+2080,values,16);}
{uint32_t values[4]={2147483647u,2147483647u,2147483647u,2147483647u};std::memcpy(code+2096,values,16);}
{unsigned char check[]={0x3b,0x0d,0,0,0,0,0x75,0x02,0xf3,0xc3,0xcc,0xc3};std::memcpy(code+2016,check,sizeof(check));}
{uint32_t address=reinterpret_cast<uint32_t>(code+2064);std::memcpy(code+2018,&address,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2064);std::memcpy(code+10,&address,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2032);std::memcpy(code+29,&address,4);}
{int32_t displacement=459;std::memcpy(code+113,&displacement,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2032);std::memcpy(code+121,&address,4);}
{int32_t displacement=376;std::memcpy(code+196,&displacement,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2032);std::memcpy(code+204,&address,4);}
{int32_t displacement=293;std::memcpy(code+279,&displacement,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2032);std::memcpy(code+290,&address,4);}
{int32_t displacement=210;std::memcpy(code+362,&displacement,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2032);std::memcpy(code+370,&address,4);}
{int32_t displacement=127;std::memcpy(code+445,&displacement,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2032);std::memcpy(code+456,&address,4);}
{int32_t displacement=44;std::memcpy(code+528,&displacement,4);}
{int32_t displacement=1050;std::memcpy(code+546,&displacement,4);}
{int32_t displacement=1453;std::memcpy(code+559,&displacement,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2032);std::memcpy(code+697,&address,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2032);std::memcpy(code+952,&address,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2032);std::memcpy(code+1201,&address,4);}
{int32_t displacement=56;std::memcpy(code+1428,&displacement,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2032);std::memcpy(code+1545,&address,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2048);std::memcpy(code+1558,&address,4);}
{int32_t displacement=20;std::memcpy(code+1608,&displacement,4);}
{int32_t displacement=133;std::memcpy(code+1623,&displacement,4);}
{int32_t displacement=141;std::memcpy(code+1743,&displacement,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2080);std::memcpy(code+1786,&address,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2080);std::memcpy(code+1797,&address,4);}
{int32_t displacement=7;std::memcpy(code+1877,&displacement,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2080);std::memcpy(code+1891,&address,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2080);std::memcpy(code+1912,&address,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2080);std::memcpy(code+1934,&address,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2096);std::memcpy(code+1956,&address,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2096);std::memcpy(code+1970,&address,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+2096);std::memcpy(code+1987,&address,4);}
}
inline constexpr unsigned projectionChainAllocationSize=2112;
