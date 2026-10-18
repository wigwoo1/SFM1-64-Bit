#pragma once
#include "engine_projection_inverse_native_bytes.h"
inline void initializeProjectionInverseNative(unsigned char* code){
std::memcpy(code+0,inverseChainBytes,822);
{uint32_t values[4]={1065353216u,1065353216u,1065353216u,1065353216u};std::memcpy(code+848,values,16);}
{uint32_t values[4]={2147483647u,2147483647u,2147483647u,2147483647u};std::memcpy(code+864,values,16);}
{uint32_t values[4]={2147483648u,2147483648u,2147483648u,2147483648u};std::memcpy(code+880,values,16);}
{uint32_t values[4]={897988541u,897988541u,897988541u,897988541u};std::memcpy(code+896,values,16);}
{uint32_t values[4]={3141592654u,3141592654u,3141592654u,3141592654u};std::memcpy(code+912,values,16);}
{unsigned char check[]={0x3b,0x0d,0,0,0,0,0x75,0x02,0xf3,0xc3,0xcc,0xc3};std::memcpy(code+832,check,sizeof(check));}
{uint32_t address=reinterpret_cast<uint32_t>(code+912);std::memcpy(code+834,&address,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+912);std::memcpy(code+10,&address,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+848);std::memcpy(code+29,&address,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+864);std::memcpy(code+140,&address,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+880);std::memcpy(code+148,&address,4);}
{uint32_t address=reinterpret_cast<uint32_t>(code+896);std::memcpy(code+156,&address,4);}
{int32_t displacement=32;std::memcpy(code+796,&displacement,4);}
{int32_t displacement=14;std::memcpy(code+814,&displacement,4);}
}
inline constexpr unsigned projectionInverseAllocationSize=928;
