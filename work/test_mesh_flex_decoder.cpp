#include "mdl_mesh_flex_candidate.h"
#include <fstream>
#include <iostream>
int main(){try{std::ifstream file(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen.mdl)",std::ios::binary);if(!file)throw std::runtime_error("model missing");std::vector<uint8_t>bytes((std::istreambuf_iterator<char>(file)),{});auto flexes=readMdl49HalfMeshFlexesCandidate(bytes,47850);size_t count=0;for(const auto&f:flexes)for(const auto&v:f.vertices){if(uint64_t(f.firstVertex)+v.index>=47850)throw std::runtime_error("mapped VVD vertex outside buffer");++count;}if(flexes.size()!=120||count!=101970)throw std::runtime_error("installed flex count mismatch");
 auto read=[&](size_t p){int32_t v;std::memcpy(&v,bytes.data()+p,4);return v;};size_t mesh=33708,flex=mesh+read(mesh+20),vertex=flex+read(flex+24);unsigned rejected=0;
 for(auto mutation:std::vector<std::pair<size_t,uint32_t>>{{flex,UINT32_MAX},{flex+20,UINT32_MAX},{flex+24,0x80000000},{flex+32,1},{152,0x4000},{vertex,0xffffffff}}){auto bad=bytes;std::memcpy(bad.data()+mutation.first,&mutation.second,4);bool failed=false;try{readMdl49HalfMeshFlexesCandidate(bad,47850);}catch(const std::runtime_error&){failed=true;}if(!failed)throw std::runtime_error("invalid mesh flex accepted");++rejected;}
 std::cout<<"installed mesh flex decode passed flexes="<<flexes.size()<<" vertices="<<count<<" VVD_mapping_in_bounds malformed_rejected="<<rejected<<" native_encoding_blending=unverified\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
