// Owned offline hardware-cap propagation harness; no original runtime initialization.
#define main precedingSourceSamplingMain
#include "probe_source_texture_sampling_x86.cpp"
#undef main
#include <d3d9.h>
#include <cstddef>
#include <algorithm>
static_assert(offsetof(D3DCAPS9,TextureFilterCaps)==0x40&&offsetof(D3DCAPS9,MaxAnisotropy)==0x6c,"actual Windows D3DCAPS9 field layout");
static_assert(D3DPTFILTERCAPS_MINFANISOTROPIC==0x400&&D3DPTFILTERCAPS_MAGFANISOTROPIC==0x4000000,"actual Windows anisotropy capability masks");
struct CapsFixture:Fixture {
    std::array<uintptr_t,8>commandTable{};uintptr_t commandObject=0;
    std::array<uint32_t,20>parent{};uint32_t requested=0,result=0,gets=0,parms=0;
    inline static CapsFixture*current=nullptr;
    static void*__cdecl commandLine(){++current->gets;return &current->commandObject;}
    static int __fastcall parm(void*p,void*,const char*name,int fallback){need(p==&current->commandObject&&name&&std::string(name)=="-maxdxlevel"&&uint32_t(fallback)==current->requested,"actual maxdxlevel query");++current->parms;return int(current->result);}
    explicit CapsFixture(const char*path):Fixture(path,false){current=this;commandTable.fill(reinterpret_cast<uintptr_t>(&forbidden));commandTable[7]=reinterpret_cast<uintptr_t>(&parm);commandObject=reinterpret_cast<uintptr_t>(commandTable.data());put(0xfa234,reinterpret_cast<uintptr_t>(&commandLine));put(0x183c0c,reinterpret_cast<uintptr_t>(parent.data()));}
};
int main(int argc,char**argv){try{
    need(sizeof(void*)==4&&argc==4&&!std::ifstream(argv[3]).good(),"original-shader-image queries fresh-output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"caps IO");const auto count=read(in);need(count&&count<=65536,"caps case count");write(out,count);CapsFixture f(argv[1]);
    for(uint32_t serial=0;serial<count;++serial){std::array<uint32_t,7>q{};for(auto&v:q)v=read(in);need(q[1]<=3&&q[2]>=80&&q[2]<=100&&q[3]>=80&&q[3]<=100&&q[4]<=1&&q[5]<=16,"caps controlled domain");std::array<uint8_t,0x2f8>source{};source.fill(0xb6);auto*raw=source.data()+16;
        auto rawField=[&](size_t offset,uint32_t value){std::memcpy(raw+offset,&value,4);};
        // This record is an authored fixture guided by the statically authenticated
        // original D3DCAPS9 decode. The whole original driver-producer is NOT run.
        rawField(0x210,q[2]);rawField(0x214,80);rawField(0x218,100);rawField(0x22c,q[5]);rawField(0x234,(q[1]&1)?q[0]:1u);raw[0x2cc]=uint8_t((raw[0x2cc]&0xcfu)|((q[1]&3u)<<4));const auto before=source;
        std::array<uint8_t,0x8b0>guard{};guard.fill(0xa7);auto*object=guard.data()+16;uint32_t table=0x100fa6b4;std::memcpy(object,&table,4);std::array<uint32_t,12>deviceInfo{};deviceInfo[10]=q[2];const auto deviceBefore=deviceInfo;f.parent.fill(0);f.parent[12]=q[4];f.requested=q[2];f.result=q[3];f.gets=f.parms=0;using Setup=void(__thiscall*)(void*,void*,const void*);reinterpret_cast<Setup>(0x100038d0)(object,deviceInfo.data(),raw);need(f.gets==1&&f.parms==1&&source==before&&deviceInfo==deviceBefore,"whole setup services and input immutability");need(std::memcmp(object+4,raw,0x2d8)==0&&std::memcmp(object+0x5b4,raw,0x2d8)==0,"default/actual copies immutable; adjusted current copy separate");for(size_t i=0;i<16;++i)need(guard[i]==0xa7&&guard[guard.size()-16+i]==0xa7,"hardware object outer guards");
        using Get=uint32_t(__thiscall*)(void*);auto*vt=reinterpret_cast<uintptr_t*>(0x100fa6b4);auto maximum=reinterpret_cast<Get>(vt[5])(object),stages=reinterpret_cast<Get>(vt[1])(object);uint32_t level;std::memcpy(&level,object+0x4ec,4);write(out,serial);for(auto v:q)write(out,v);write(out,maximum);write(out,stages);write(out,object[0x5a8]);write(out,level);write(out,f.gets);write(out,f.parms);out.write(reinterpret_cast<const char*>(object+0x2dc),0x2d8);
    }
    need(in.peek()==EOF,"caps framing");out.flush();need(bool(out),"caps output");f.image.verify();std::cout<<"PASS original_capability_wrapper_setup_adjustment_cases="<<count<<" Windows_D3DCAPS9_layout=ASSERTED private_x86=1 driver_producer=STATIC_ONLY GPU=NOT_USED original_process=ABSENT\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
