#define NOMINMAX
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN64
#include "vtf_block_decode.h"
#else
#include <windows.h>
#include "material_construction_native_fixture.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t word(std::istream&i){uint32_t n;need(bool(i.read(reinterpret_cast<char*>(&n),4)),"input word");return n;}
static std::string text(std::istream&i){auto n=word(i);need(n<=4095,"text extent");std::string s(n,'\0');need(bool(i.read(s.data(),n)),"text bytes");return s;}
int main(int argc,char**argv){try{
    need(argc==4&&!std::ifstream(argv[3]).good(),"image queries fresh-output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"probe IO");
#ifndef _WIN64
    static_assert(sizeof(void*)==4,"private x86 reference");MaterialConstructionNativeFixture image(argv[1]);
#else
    static_assert(sizeof(void*)==8,"owned x64 decoder");(void)argv[1];
#endif
    const auto count=word(in);need(count&&count<=8192,"case extent");
    for(uint32_t serial=0;serial<count;++serial){const auto format=word(in),w=word(in),h=word(in),level=word(in),frame=word(in),n=word(in);const auto source=text(in);
        need((format==0||format==3||format==12)&&w&&h&&w<=32&&h<=32&&n==w*h*(format==3?3u:4u),"input extent");std::vector<uint8_t>raw(n);need(bool(in.read(reinterpret_cast<char*>(raw.data()),n)),"input pixel bytes");const auto before=raw;
        std::vector<uint8_t>rgba;
#ifndef _WIN64
        (void)level;(void)frame;
        std::vector<uint8_t>guarded(size_t(w)*h*4+32,0x9b);using Convert=uint32_t(__cdecl*)(const void*,uint32_t,void*,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
        const auto ok=reinterpret_cast<Convert>(0x10048ec0)(raw.data(),format,guarded.data()+16,0,w,h,0,0);need((ok&255u)!=0,"native conversion success");for(size_t i=0;i<16;++i)need(guarded[i]==0x9b&&guarded[guarded.size()-16+i]==0x9b,"native output guards");rgba.assign(guarded.begin()+16,guarded.end()-16);image.image.verify();
#else
        std::ifstream f(source,std::ios::binary);need(bool(f),"fixture input");std::vector<uint8_t>vtf((std::istreambuf_iterator<char>(f)),{});const auto original=vtf;const auto decoded=decodeVtfMipCandidate(vtf,level,frame);need(decoded.width==w&&decoded.height==h&&vtf==original,"owned dimensions/input identity");rgba=decoded.rgba;
#endif
        need(raw==before&&rgba.size()==size_t(w)*h*4,"source preservation/output extent");const std::array<uint32_t,4>record{serial,w,h,uint32_t(rgba.size())};out.write(reinterpret_cast<const char*>(record.data()),sizeof(record));out.write(reinterpret_cast<const char*>(rgba.data()),std::streamsize(rgba.size()));
    }
    need(in.peek()==EOF&&in.eof(),"query framing");out.flush();need(bool(out),"output write");std::cout<<"PASS conversion_cases="<<count<<" gpu_calls=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
