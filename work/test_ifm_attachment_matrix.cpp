#include "ifm_attachment_matrix_candidate.h"
#include "source_mdl_attachment_capacity_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "read_binary_asset_candidate.h"
#include <iostream>
#include <map>
struct AttachmentSafetyServices {
    bool available=true;int32_t count=1;unsigned setups=0;std::string order;
    IfmAttachmentDescriptorCandidate d{0,{{{1,0,0,2},{0,1,0,3},{0,0,1,4}}}};
    bool headerAvailable(){order+='H';return available;}int32_t attachmentCount(){order+='C';return count;}
    std::array<float,3>position(){order+='P';return {10,20,30};}std::array<float,3>angles(){order+='A';return {0,0,0};}
    int32_t clockTick(){order+='T';return 50000;}bool setupBones(uint32_t max,uint32_t mask,float time){order+='S';if(max||mask!=0x200||time!=5)throw std::runtime_error("setup contract");++setups;return false;}
    auto descriptor(uint32_t i){order+='D';if(i)throw std::runtime_error("descriptor index");return d;}uint32_t mappedBone(uint32_t){order+='M';return 0;}
    RecoveredMatrix3x4 boneWorld(uint32_t){order+='B';return {{{1,0,0,10},{0,1,0,20},{0,0,1,30}}};}
};
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 required");auto require=[](bool b){if(!b)throw std::runtime_error("attachment safety assertion");};
    if(argc==3){auto info=readBinaryAssetCandidate(argv[1]);auto dir=std::filesystem::path(argv[1]).parent_path();auto mounts=gameinfoMountsCandidate(std::string(info.begin(),info.end()),dir,dir.parent_path());
        auto result=sourceMdlAttachmentCapacityCandidate(argv[2],[&](const std::string&name){std::filesystem::path file;for(const auto&mount:mounts)if(std::filesystem::is_regular_file(mount/name)){file=mount/name;break;}if(file.empty())throw std::runtime_error("included model unavailable: "+name);auto bytes=readBinaryAssetCandidate(file);auto meta=readSourceMdlAttachmentCapacityMetadataCandidate(bytes);std::cout<<"asset="<<name<<" local="<<meta.localAttachments<<" includes="<<meta.includes.size()<<" file="<<file.string()<<'\n';return meta;});
        std::cout<<"PASS upper_bound="<<result.upperBound<<" expanded_groups="<<result.expandedGroups<<" exact_virtual_table=unverified\n";return 0;
    }
    require(argc==1);unsigned checks=0,rejections=0;
    AttachmentSafetyServices s;auto a=ifmAttachmentMatrixCandidate(1,s);require(a.valid&&a.matrix[0][3]==12&&a.matrix[1][3]==23&&a.matrix[2][3]==34&&s.order=="HCTSDMB");++checks;
    s={};s.available=false;a=ifmAttachmentMatrixCandidate(1,s);require(!a.valid&&s.order=="HPA"&&s.setups==0&&a.matrix[0][3]==10);++checks;
    s={};a=ifmAttachmentMatrixCandidate(0,s);require(!a.valid&&s.order=="HPA");++checks;
    s={};a=ifmAttachmentMatrixCandidate(2,s);require(!a.valid&&s.order=="HCPA");++checks;
    s={};s.d.flags=0x10000;s.d.local[0][0]=NAN;a=ifmAttachmentMatrixCandidate(1,s);require(a.valid&&a.matrix[0][0]==1&&a.matrix[0][3]==12);++checks;
    auto reject=[&](auto fn){try{fn();}catch(const std::runtime_error&){++rejections;return;}throw std::runtime_error("invalid accepted");};
    s={};s.d.local[0][0]=NAN;reject([&]{(void)ifmAttachmentMatrixCandidate(1,s);});
    std::map<std::string,SourceMdlAttachmentCapacityMetadataCandidate>meta{{"root.mdl",{10,{"child.mdl","child.mdl"}}},{"child.mdl",{2,{}}}};
    auto load=[&](const std::string&n){return meta.at(n);};
    auto cap=sourceMdlAttachmentCapacityCandidate("ROOT.MDL",load);require(cap.upperBound==14&&cap.expandedGroups==3);++checks;
    meta["child.mdl"].includes={"root.mdl"};reject([&]{(void)sourceMdlAttachmentCapacityCandidate("root.mdl",load);});
    for(const auto&name:{"../root.mdl","C:/root.mdl","/root.mdl","root.txt","root//child.mdl"})reject([&]{(void)sourceMdlIncludeIdentityCandidate(name);});
    std::vector<uint8_t>b(0x158);auto put=[&](size_t o,int32_t v){std::memcpy(b.data()+o,&v,4);};put(0,0x54534449);put(4,49);put(76,int32_t(b.size()));require(readSourceMdlAttachmentCapacityMetadataCandidate(b).localAttachments==0);++checks;
    put(0xf0,1);reject([&]{(void)readSourceMdlAttachmentCapacityMetadataCandidate(b);});put(0xf0,0);put(0x150,65);reject([&]{(void)readSourceMdlAttachmentCapacityMetadataCandidate(b);});
    std::cout<<"PASS checks="<<checks<<" rejections="<<rejections<<" capacity_is_upper_bound_only=1\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
