#include "source_bsp_hdr_enable_candidate.h"
#include "source_bsp_ambient_selection_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>

int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 required");
    auto require=[](bool ok,const char*message){if(!ok)throw std::runtime_error(message);};
    size_t checks=0;
    // Explicit callback order/short circuits. Invalid sizes belong to the
    // loader, not to this byte-compatible decision policy.
    struct Case {int32_t version,a,b,c,level,type;bool capable,request;const char*order;};
    for(const auto&c:std::array<Case,10>{{
        {20,0,1,1,2,2,false,false,"AC"},
        {20,1,0,1,2,2,false,false,"ABC"},
        {20,1,1,0,2,2,false,false,"ABC"},
        {19,1,1,0,2,2,true,true,"ABLT"},
        {20,1,1,-1,2,2,true,true,"ABCLT"},
        {20,1,1,1,1,2,true,false,"ABCL"},
        {20,1,1,1,2,0,true,false,"ABCLT"},
        {20,1,1,1,2,-1,true,true,"ABCLT"},
        {-1,1,1,0,2,2,true,true,"ABLT"},
        {21,-1,1,1,2,2,false,false,"AC"}
    }}){
        std::string order;const std::array<int32_t,3>lengths{c.a,c.b,c.c};
        auto r=sourceBspHdrEnableRequestCandidate(c.version,[&](unsigned l){order+=char('A'+l-53);return lengths.at(l-53);},[&]{order+='L';return c.level;},[&]{order+='T';return c.type;});
        require(r.mapHasHdr==c.capable&&r.enableRequested==c.request&&order==c.order,"enable branch/order");++checks;
    }
    require(argc==2,"map.bsp required");std::ifstream f(argv[1],std::ios::binary);require(bool(f),"map read");std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});require(b.size()>=1036,"BSP header");
    auto word=[&](size_t o){uint32_t v;std::memcpy(&v,b.data()+o,4);return v;};
    require(word(0)==0x50534256&&word(4)==20&&word(8+10*16+8)==1,"benchmark BSP20 split leaves");
    auto length=[&](unsigned id){auto n=word(8+id*16+4);require(n<=INT32_MAX,"valid BSP length");return int32_t(n);};
    struct Configuration {int32_t level,dx,type,emulate;bool request;SourceBspAmbientStorageCandidate storage;};
    const auto H=SourceBspAmbientStorageCandidate::SplitHdr,L=SourceBspAmbientStorageCandidate::SplitLdr;
    const std::array<Configuration,8>configs{{{0,95,2,0,false,L},{1,95,2,0,false,L},{2,95,2,0,true,H},{3,95,1,0,true,H},
        {2,89,2,0,true,L},{2,95,0,0,false,L},{0,95,2,1,false,H},{2,90,2,0,true,H}}};
    for(size_t i=0;i<configs.size();++i){const auto&c=configs[i];
        const auto result=sourceBspHdrEnableRequestCandidate(20,length,[&]{return c.level;},[&]{return c.type;});
        require(result.mapHasHdr&&result.enableRequested==c.request,"benchmark map request");
        // Explicit owned state handoff, not execution/proof of the native
        // hardware setter, material notifications or device setup lifecycle.
        struct Hardware {bool enabled;int32_t dx,type;
            void enableGlobalHdr(){enabled=true;}bool hdrEnabled(){return enabled;}
            int32_t dxSupportLevel(){return dx;}int32_t hardwareHdrType(){return type;}
        }hardware{result.enableRequested,c.dx,c.type};
        const auto selected=sourceBspAmbientSelectionCandidate(1,[&]{return sourceHardwareHdrTypeCandidate(c.emulate,hardware);},[&]{return length(55);});
        require(selected==c.storage,"benchmark configuration handoff selection");
        SourceBspLeafAmbientCandidate ambient(b,sourceBspSplitAmbientEncodingCandidate(selected));
        require(ambient.samples.size()==(selected==H?12696u:2842u),"benchmark selected sample population");++checks;
        std::cout<<"configuration="<<i<<" mat_hdr_level="<<c.level<<" dx="<<c.dx<<" hardware_type="<<c.type<<" emulate_gl="<<c.emulate<<" requested="<<result.enableRequested<<" selected="<<(selected==H?"hdr":"ldr")<<" samples="<<ambient.samples.size()<<'\n';
    }
    std::cout<<"PASS checks="<<checks<<" explicit_configurations="<<configs.size()<<" native_runtime_configuration=unverified editor_integration=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
