#include "source_bsp_collision_records_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("map output required");std::ifstream in(argv[1],std::ios::binary);if(!in)throw std::runtime_error("map unavailable");std::vector<uint8_t>b((std::istreambuf_iterator<char>(in)),{});SourceBspWorldCollisionDataCandidate data(b);std::vector<uint32_t>surfaces(32768);for(size_t i=0;i<surfaces.size();++i)surfaces[i]=uint32_t((i&255)*257);
 const auto records=sourceBspCollisionRecordsCandidate(data,surfaces);std::ofstream out(argv[2],std::ios::binary);size_t count=0;for(const auto&r:records)if(r.optimized){out.write(reinterpret_cast<const char*>(r.box.data()),48);++count;}out.flush();if(!out)throw std::runtime_error("output failure");std::cout<<"PASS brushes="<<records.size()<<" boxes="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
