#include "source_bsp_displacement_data_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("map output required");std::ifstream in(argv[1],std::ios::binary);if(!in)throw std::runtime_error("map unavailable");std::vector<uint8_t>b((std::istreambuf_iterator<char>(in)),{});SourceBspDisplacementDataCandidate data(b);std::ofstream out(argv[2],std::ios::binary);uint32_t count=uint32_t(data.records.size());out.write(reinterpret_cast<char*>(&count),4);for(const auto&r:data.records){out.write(reinterpret_cast<const char*>(r.raw.data()),176);for(size_t i=0;i<r.vertexCount;++i)out.write(reinterpret_cast<const char*>(data.vertices[r.vertexStart+i].data()),20);out.write(reinterpret_cast<const char*>(data.triangleTags.data()+r.triangleStart),2*r.triangleCount);}out.flush();if(!out)throw std::runtime_error("output failure");std::cout<<"PASS records="<<data.records.size()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
