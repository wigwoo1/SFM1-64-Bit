#include <windows.h>
#include "sfm_time_to_frame_candidate.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv) {try {
    if(argc!=2)throw std::runtime_error("oracle path required");
#ifdef _M_IX86
    if(std::ifstream(argv[1],std::ios::binary).good())throw std::runtime_error("oracle exists");
    std::ifstream input("work/time-to-frame-original-code.bin",std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(input)),{});
    if(bytes.empty())throw std::runtime_error("missing verified code bundle");
    auto* code=VirtualAlloc(nullptr,bytes.size(),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!code)throw std::runtime_error("allocation failed");
    std::memcpy(code,bytes.data(),bytes.size());DWORD old=0;
    if(!VirtualProtect(code,bytes.size(),PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");
    FlushInstructionCache(GetCurrentProcess(),code,bytes.size());
    using Native=std::int32_t(__thiscall*)(std::int32_t*,std::uint32_t,unsigned);
    std::ofstream stream(argv[1],std::ios::binary);
#else
    std::ifstream stream(argv[1],std::ios::binary);
#endif
    if(!stream)throw std::runtime_error("oracle unavailable");
    unsigned checks=0;
    const SfmFrameRateCandidate rates[]={{24,10000},{30,10010},{60,10010},{1,1},{65535,1},{0,10000},{7,9999},{65535,65535}};
    for(auto rate:rates)for(int t=-20000;t<=20000;++t)for(auto mode:{0u,1u,2u,3u,99u}) {
        std::int32_t actual=0;
#ifdef _M_IX86
        const auto packed=static_cast<std::uint32_t>(rate.numerator)|(static_cast<std::uint32_t>(rate.timeUnits)<<16);
        auto time=t;actual=reinterpret_cast<Native>(code)(&time,packed,mode);
        if(time!=t)throw std::runtime_error("original input changed");
        stream.write(reinterpret_cast<const char*>(&actual),4);
#else
        actual=sfmTimeToFrameCandidate(t,rate,mode);std::int32_t expected=0;
        stream.read(reinterpret_cast<char*>(&expected),4);
        if(!stream || actual!=expected)throw std::runtime_error("original parity mismatch");
#endif
        ++checks;
    }
#ifdef _M_IX86
    stream.close();VirtualFree(code,0,MEM_RELEASE);
#else
    if(stream.peek()!=std::char_traits<char>::eof())throw std::runtime_error("oracle trailing bytes");
#endif
    std::cout<<"time-to-frame original parity cases="<<checks<<" passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
