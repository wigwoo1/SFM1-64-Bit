#include "mdl_flex_rules_candidate.h"
#include <fstream>
#include <iostream>
#include <map>
int main(int argc,char**argv){try{
 std::vector<uint8_t>fixture(512);auto put=[&](size_t p,uint32_t v){std::memcpy(fixture.data()+p,&v,4);};put(0,0x54534449);put(4,49);put(76,512);put(260,2);put(264,436);put(276,1);put(280,408);put(408,1);put(412,2);put(416,12);put(420,1);put(424,0x3f800000);put(428,2);put(432,7);
 auto parsed=readMdl49FlexRulesCandidate(fixture);if(parsed.size()!=1||parsed[0].target!=1||parsed[0].operations.size()!=2||parsed[0].operations[0].operandBits!=0x3f800000||parsed[0].operations[1].opcode!=2)throw std::runtime_error("valid rule mismatch");
 unsigned rejected=0;for(auto mutation:std::vector<std::pair<size_t,uint32_t>>{{408,2},{412,UINT32_MAX},{416,512},{276,UINT32_MAX},{280,511},{264,511}}){auto saved=fixture;put(mutation.first,mutation.second);bool failed=false;try{readMdl49FlexRulesCandidate(fixture);}catch(const std::runtime_error&){failed=true;}fixture=std::move(saved);if(!failed)throw std::runtime_error("malformed rule accepted");++rejected;}
 std::cout<<"flex rule parser valid fixture passed malformed_rejected="<<rejected<<'\n';
 for(int i=1;i<argc;++i){std::ifstream file(argv[i],std::ios::binary);if(!file)throw std::runtime_error("model missing");std::vector<uint8_t>bytes((std::istreambuf_iterator<char>(file)),{});auto rules=readMdl49FlexRulesCandidate(bytes);std::map<int32_t,size_t>histogram;for(const auto&r:rules)for(const auto&op:r.operations)++histogram[op.opcode];std::cout<<"model="<<argv[i]<<" rules="<<rules.size();for(const auto&item:histogram)std::cout<<" opcode"<<item.first<<'='<<item.second;std::cout<<'\n';}
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
