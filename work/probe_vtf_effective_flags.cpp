#include "vtf_effective_flags_candidate.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"owned x64 helper");
    if(argc!=3||std::ifstream(argv[2]).good())throw std::runtime_error("query fresh-output");
    std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("IO");
    std::array<uint32_t,3>query{};uint32_t count=0;
    while(in.read(reinterpret_cast<char*>(query.data()),sizeof(query))){
        uint32_t value=0xaabbccddu;const auto before=query;const bool accepted=vtfEffectiveFlagsCandidate(query[0],query[1],query[2],value);
        if(query!=before)throw std::runtime_error("input mutation");std::array<uint32_t,2>record{uint32_t(accepted),value};out.write(reinterpret_cast<const char*>(record.data()),sizeof(record));++count;
    }
    if(!in.eof()||in.gcount()||!count)throw std::runtime_error("framing");out.flush();if(!out)throw std::runtime_error("output");std::cout<<"PASS owned_flag_cases="<<count<<" original_runtime=ABSENT editor=NOT_CONNECTED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
