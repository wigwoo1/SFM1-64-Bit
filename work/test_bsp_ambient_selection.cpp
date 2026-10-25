#include "source_bsp_ambient_selection_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 required");
    auto require=[](bool ok,const char*message){if(!ok)throw std::runtime_error(message);};
    unsigned checks=0,rejected=0,queries=0,lengthQueries=0;
    auto select=[&](uint32_t version,int type,int length){return sourceBspAmbientSelectionCandidate(version,[&]{++queries;return type;},[&]{++lengthQueries;return length;});};
    require(select(0,2,28)==SourceBspAmbientStorageCandidate::LegacyLeafV0&&queries==0&&lengthQueries==0,"legacy queried split settings");++checks;
    require(select(1,0,28)==SourceBspAmbientStorageCandidate::SplitLdr&&queries==1&&lengthQueries==0,"disabled HDR queried sample length");++checks;
    for(int type:{-1,1,2})for(int length:{-1,0,1,28}){const auto expected=length>0?SourceBspAmbientStorageCandidate::SplitHdr:SourceBspAmbientStorageCandidate::SplitLdr;require(select(1,type,length)==expected,"signed type/size branch");++checks;}
    require(sourceEffectiveBspLumpLengthCandidate(true,100,0)==0&&sourceEffectiveBspLumpLengthCandidate(true,100,-1)==-1&&sourceEffectiveBspLumpLengthCandidate(false,-1,100)==-1,"override priority");++checks;
    for(uint32_t version:{2u,255u,0xffffffffu}){const auto oldQueries=queries;try{(void)select(version,1,28);}catch(const std::invalid_argument&){++rejected;require(queries==oldQueries,"invalid version queried hardware");continue;}throw std::runtime_error("unsupported version accepted");}
    for(auto storage:{SourceBspAmbientStorageCandidate::LegacyLeafV0,static_cast<SourceBspAmbientStorageCandidate>(99)}){try{(void)sourceBspSplitAmbientEncodingCandidate(storage);}catch(const std::invalid_argument&){++rejected;continue;}throw std::runtime_error("unsupported storage became split LDR");}
    require(argc==2,"map.bsp required");std::ifstream f(argv[1],std::ios::binary);require(bool(f),"map read");std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});require(b.size()>=1036,"BSP header");
    auto word=[&](size_t o){uint32_t v;std::memcpy(&v,b.data()+o,4);return v;};require(word(0)==0x50534256&&word(4)==20,"BSP20 required");
    const auto leafVersion=word(8+10*16+8);const auto hdrBytes=word(8+55*16+4);require(hdrBytes<=INT32_MAX,"HDR byte length");
    auto hdrSelection=select(leafVersion,2,int32_t(hdrBytes));auto ldrSelection=select(leafVersion,0,int32_t(hdrBytes));
    SourceBspLeafAmbientCandidate hdr(b,sourceBspSplitAmbientEncodingCandidate(hdrSelection)),ldr(b,sourceBspSplitAmbientEncodingCandidate(ldrSelection));
    require(hdr.encoding==SourceBspAmbientEncodingCandidate::Hdr&&ldr.encoding==SourceBspAmbientEncodingCandidate::Ldr&&hdr.samples.size()!=ldr.samples.size(),"real datasets not selected independently");++checks;
    // Simulate an active EMPTY external sample-lump override. Selection falls
    // to split LDR even though the base BSP contains HDR sample bytes.
    require(select(leafVersion,2,sourceEffectiveBspLumpLengthCandidate(true,int32_t(hdrBytes),0))==SourceBspAmbientStorageCandidate::SplitLdr,"empty override ignored");++checks;
    std::cout<<"PASS checks="<<checks<<" rejections="<<rejected<<" hdr_samples="<<hdr.samples.size()<<" ldr_samples="<<ldr.samples.size()<<" explicit_hardware_query=1 no_legacy_fallback=1\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
