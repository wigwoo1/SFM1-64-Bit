// Conditional policy checks for the user's temporary reference configuration.
// No application configuration, original machine code, native DLL or GPU calls.
#include "shader_skin_material_predicate_candidate.h"
#include "source_bsp_hdr_enable_candidate.h"
#include "source_bsp_ambient_selection_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>

int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 required");
    auto need=[](bool value,const char*why){if(!value)throw std::runtime_error(why);};
    need(argc==5,"map.bsp assumed-dx mat-hdr-level mat-phong required");
    const int dx=std::stoi(argv[2]),hdr=std::stoi(argv[3]),phong=std::stoi(argv[4]);
    need(dx==95&&hdr==3&&phong==1,"temporary user profile changed: review this audit explicitly");
    std::ifstream file(argv[1],std::ios::binary);need(bool(file),"benchmark map read");
    std::vector<uint8_t>bytes((std::istreambuf_iterator<char>(file)),{});
    auto word=[&](size_t offset){need(offset<=bytes.size()&&bytes.size()-offset>=4,"BSP header bounds");int32_t n;std::memcpy(&n,bytes.data()+offset,4);return n;};
    need(word(0)==0x50534256&&word(4)==20&&word(8+10*16+8)==1,"benchmark BSP20 split leaves");
    auto length=[&](unsigned id){return word(8+id*16+4);};
    need(length(53)>0&&length(54)>0&&length(55)>0,"base-header HDR lumps missing");
    unsigned cases=0;
    // DX90/92 are comparison cases, never substituted for the selected95.
    // Typed texture/material inputs below are explicit fixtures, not raw VMT inference.
    for(int level:{90,92,dx})for(int type:{0,1,2})for(int emulate:{0,1})for(int materialPhong:{0,phong}){
        ShaderSkinMaterialInputsCandidate material{materialPhong,1,uint8_t(3),1,true};
        const auto skin=shaderSkinMaterialPredicateCandidate(material);
        const bool skinRoute=shaderSkinDrawGateCandidate(skin,level);
        need(skinRoute==(materialPhong!=0&&level>=92),"typed material hardware gate");
        auto request=sourceBspHdrEnableRequestCandidate(20,length,[&]{return hdr;},[&]{return type;});
        auto levelTwo=sourceBspHdrEnableRequestCandidate(20,length,[]{return 2;},[&]{return type;});
        need(request.mapHasHdr&&request.enableRequested==(type!=0)&&request.enableRequested==levelTwo.enableRequested,"HDR3 enable-request policy");
        struct Hardware {
            bool enabled;int level,type;
            void enableGlobalHdr(){enabled=true;}bool hdrEnabled(){return enabled;}
            int dxSupportLevel(){return level;}int hardwareHdrType(){return type;}
        }hardware{request.enableRequested,level,type};
        const int current=sourceHardwareHdrTypeCandidate(emulate,hardware);
        auto selected=sourceBspAmbientSelectionCandidate(1,[&]{return current;},[&]{return length(55);});
        need(selected==(type?SourceBspAmbientStorageCandidate::SplitHdr:SourceBspAmbientStorageCandidate::SplitLdr),"conditional ambient storage");
        std::cout<<"{\"dx\":"<<level<<",\"mat_hdr_level\":"<<hdr<<",\"mat_phong\":"<<materialPhong<<",\"fixture_hardware_hdr_type\":"<<type<<",\"fixture_emulate_gl\":"<<emulate<<",\"fixture_alias_global_query\":true,\"phong_draw_gate\":"<<skinRoute<<",\"hdr_request\":"<<request.enableRequested<<",\"ambient_storage\":\""<<(type?"hdr":"ldr")<<"\"}\n";
        ++cases;
    }
    need(cases==36,"profile scenario coverage");
    std::cout<<"{\"status\":\"VERIFIED\",\"conditional_scenarios\":"<<cases<<",\"actual_native_configuration\":\"PARTIALLY VERIFIED: user-reported controls and assumed DX95\",\"editor_changed\":false}\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
