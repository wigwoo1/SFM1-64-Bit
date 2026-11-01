#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "material_construction_native_fixture.h"
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==4,"private x86 primitive reference");need(argc==3&&!std::ifstream(argv[2]).good(),"image fresh-output");MaterialConstructionNativeFixture image(argv[1]);image.begin(true);
    std::ofstream out(argv[2],std::ios::binary);need(bool(out),"output");uint32_t count=0;
    using Size=uint32_t(__cdecl*)(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t*);
    using Convert=uint32_t(__cdecl*)(const void*,uint32_t,void*,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
    for(uint32_t format:{13u,15u,20u})for(uint32_t dimension:{1u,2u,4u,8u,16u}){
        const auto size=reinterpret_cast<Size>(0x10049790)(dimension,dimension,1,1,format,nullptr);
        const uint32_t expected=format==20?0u:((dimension+3)/4)*((dimension+3)/4)*(format==15?16u:8u);need(size==expected,"measured native size");
        for(uint32_t target:{0u,3u,12u}){
            const size_t sourceSize=size_t((dimension+3)/4)*((dimension+3)/4)*(format==15?16u:8u);std::vector<uint8_t>source(sourceSize+32,0x9b);
            std::fill(source.begin()+16,source.end()-16,uint8_t(0));const auto original=source;
            std::vector<uint8_t>destination(size_t(dimension)*dimension*(target==3?3u:4u)+32,0x9b);std::fill(destination.begin()+16,destination.end()-16,uint8_t(0xa5));const auto before=destination;
            const auto ok=reinterpret_cast<Convert>(0x10048ec0)(source.data()+16,format,destination.data()+16,target,dimension,dimension,0,0)&255u;
            need(source==original,"source guard/content unchanged");for(size_t i=0;i<16;++i)need(destination[i]==0x9b&&destination[destination.size()-16+i]==0x9b,"destination guards");
            need(format==20?(ok==0&&destination==before):ok==1,"primitive success/refusal");
            const std::array<uint32_t,6>record{format,dimension,target,size,ok,uint32_t(destination==before)};out.write(reinterpret_cast<const char*>(record.data()),sizeof(record));++count;image.image.verify();
        }
    }
    out.flush();need(bool(out),"trace write");image.image.verify();std::cout<<"PASS native_block_primitive_cases="<<count<<" format20_zero_size_and_conversion_refusal=1 gpu_calls=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
