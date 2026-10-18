#define NOMINMAX
#include <windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include "native_displacement_position_bytes.h"
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x420000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("base unavailable");for(const auto&f:positionFragments)std::memcpy(base+f.offset,f.bytes,f.size);float one=1;std::memcpy(base+0x31bdec,&one,4);DWORD old;if(!VirtualProtect(base,0x320000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x420000);using Function=void(__thiscall*)(void*);auto generate=reinterpret_cast<Function>(base+0x1415d0);
 std::array<uint32_t,13>query{};size_t cases=0;while(in.read(reinterpret_cast<char*>(query.data()),sizeof(query))){uint32_t power=query[0];if(power<1||power>4)throw std::runtime_error("unsupported fixture power");uint32_t width=(1u<<power)+1,count=width*width;std::vector<std::array<uint8_t,20>>offsets(count);if(!in.read(reinterpret_cast<char*>(offsets.data()),std::streamsize(count*20)))throw std::runtime_error("truncated fixture offsets");std::array<uint8_t,0x310>object{};std::vector<std::array<uint8_t,224>>vertices(count);std::memcpy(object.data()+12,&power,4);std::memcpy(object.data()+24,query.data()+1,48);uint32_t vertexPtr=reinterpret_cast<uint32_t>(vertices.data());std::memcpy(object.data()+0x2f8,&vertexPtr,4);for(size_t i=0;i<count;++i)std::memcpy(vertices[i].data(),offsets[i].data(),16);generate(object.data());out.write(reinterpret_cast<char*>(&count),4);for(const auto&v:vertices){for(size_t a=0;a<3;++a){float value;std::memcpy(&value,v.data()+40+4*a,4);if(!std::isfinite(value))throw std::runtime_error("nonfinite native position");}out.write(reinterpret_cast<const char*>(v.data()+40),12);}++cases;
 }if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failed");for(const auto&f:positionFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("original code changed");VirtualFree(base,0,MEM_RELEASE);std::cout<<"PASS native grids="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
