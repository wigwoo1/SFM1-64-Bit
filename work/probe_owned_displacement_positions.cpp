#include "source_displacement_grid_positions_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input output required");
 std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);
 if(!in||!out)throw std::runtime_error("workspace files");
 std::array<uint32_t,13>query{};size_t cases=0;
 while(in.read(reinterpret_cast<char*>(query.data()),sizeof(query))){
  const uint32_t power=query[0];if(power<1||power>4)throw std::runtime_error("unsupported fixture power");
  const uint32_t width=(1u<<power)+1,count=width*width;
  std::array<SourceDisplacementGridPointCandidate,4>corners{};std::memcpy(corners.data(),query.data()+1,48);
  std::vector<SourceDisplacementGridOffsetCandidate>offsets(count);
  for(auto&offset:offsets){std::array<float,5>raw{};if(!in.read(reinterpret_cast<char*>(raw.data()),20))throw std::runtime_error("truncated fixture offsets");for(size_t a=0;a<3;++a)offset.direction[a]=raw[a];offset.distance=raw[3];}
  const auto positions=sourceDisplacementGridPositionsCandidate(power,corners,offsets);
  out.write(reinterpret_cast<const char*>(&count),4);
  for(const auto&point:positions)out.write(reinterpret_cast<const char*>(point.data()),12);
  ++cases;
 }
 if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failed");
 std::cout<<"PASS owned grids="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
