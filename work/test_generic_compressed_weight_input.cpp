#include "generic_compressed_weight_input_candidate.h"
#include "studio_skin_declaration_candidate.h"
#include <fstream>
#include <iostream>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 required");unsigned checks=0,refusals=0;
    auto require=[](bool good,const char*why){if(!good)throw std::runtime_error(why);};
    require(genericCompressedWeightInputCandidate({255,255,255,127})==std::array<float,2>{0,1},"biased signed shorts");++checks;
    const auto negative=genericCompressedWeightInputCandidate({0,128,0,0});require(negative[0]<0&&negative[1]==1.0f/32768.0f,"unwanted normalization/clamp");++checks;
    for(uint64_t format:{0ull,0x83ull|(3ull<<12),0x83ull|(1ull<<12)|0x800ull,0x83ull|(1ull<<12)|(5ull<<18),0x83ull|(1ull<<12)|(1ull<<15)}){
        try{studioSkinDeclarationCandidate(format,95,false);}catch(const std::invalid_argument&){++refusals;continue;}throw std::runtime_error("declaration domain admitted");}
    require(argc==2,"fixture required");std::ifstream in(argv[1],std::ios::binary);require(bool(in),"fixture open");std::array<uint8_t,12>q{};size_t cases=0;
    while(in.read(reinterpret_cast<char*>(q.data()),12)){std::array<uint8_t,4>packed{};std::memcpy(packed.data(),q.data(),4);const auto result=genericCompressedWeightInputCandidate(packed);require(std::memcmp(result.data(),q.data()+4,8)==0,"shader token unpack reference differs");++cases;}
    require(in.eof()&&!in.gcount()&&cases>0,"fixture framing");std::cout<<"PASS checks="<<checks<<" refusals="<<refusals<<" shader_input_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
