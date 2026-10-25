#include "vmt_keyvalues_candidate.h"
#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
// Authored request inventory only. Never assigns runtime material type/state.
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 required");if(argc!=3||std::ifstream(argv[2],std::ios::binary).good())throw std::runtime_error("VMT fresh-output required");
    std::ifstream in(argv[1],std::ios::binary);if(!in)throw std::runtime_error("VMT open");const std::string raw((std::istreambuf_iterator<char>(in)),{});const auto root=parseVmtCandidate(raw);
    if(vmtAsciiLowerCandidate(root.key)!="vertexlitgeneric")throw std::runtime_error("authored shader family changed");std::ofstream out(argv[2],std::ios::binary);if(!out)throw std::runtime_error("inventory output");
    auto number=[&](uint32_t n){out.write(reinterpret_cast<const char*>(&n),4);};auto text=[&](const std::string&s){number(uint32_t(s.size()));out.write(s.data(),std::streamsize(s.size()));};
    text(root.key);for(const char*key:{"$phong","$lightwarptexture","$basemapalphaphongmask","$bumpmap"}){const auto values=vmtValuesCandidate(root,key);text(key);number(uint32_t(values.size()));for(const auto&value:values)text(value);}
    out.flush();if(!out)throw std::runtime_error("inventory write");std::cout<<"PASS authored_keys=4 effective_state=UNVERIFIED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
