#include "source_displacement_grid_triangles_candidate.h"
#include <functional>
#include <iostream>
int main(){try{
 const std::array<std::array<uint16_t,3>,8>expected={{{0,3,4},{0,4,1},{1,4,2},{2,4,5},{3,6,4},{4,6,7},{4,7,8},{4,8,5}}};
 std::vector<uint16_t>tags(8);for(size_t i=0;i<tags.size();++i)tags[i]=uint16_t(0xfff0+i);
 const auto result=sourceDisplacementGridTrianglesCandidate(1,tags);
 for(size_t i=0;i<expected.size();++i)if(result[i].vertices!=expected[i]||result[i].tags!=tags[i])throw std::runtime_error("analytical checkerboard/tag mismatch");
 size_t rejections=0;auto rejects=[&](const std::function<void()>&call){try{call();}catch(const std::runtime_error&){++rejections;return;}throw std::runtime_error("invalid input accepted");};
 rejects([&]{sourceDisplacementGridTrianglesCandidate(0,tags);});
 rejects([&]{sourceDisplacementGridTrianglesCandidate(32,tags);});
 rejects([&]{sourceDisplacementGridTrianglesCandidate(2,tags);});
 tags.push_back(0);rejects([&]{sourceDisplacementGridTrianglesCandidate(1,tags);});
 if(rejections!=4)throw std::runtime_error("rejection coverage");
 for(uint32_t power=1;power<=4;++power){const uint32_t cells=1u<<power,width=cells+1;const auto ts=sourceDisplacementGridTrianglesCandidate(power,std::vector<uint16_t>(cells*cells*2,0xffff));int area=0;for(const auto&t:ts){const int ax=t.vertices[0]%width,ay=t.vertices[0]/width,bx=t.vertices[1]%width,by=t.vertices[1]/width,cx=t.vertices[2]%width,cy=t.vertices[2]/width;const int cross=(bx-ax)*(cy-ay)-(by-ay)*(cx-ax);if(cross!=-1)throw std::runtime_error("grid winding/area");area-=cross;}if(area!=int(2*cells*cells))throw std::runtime_error("grid total area");}
 std::cout<<"PASS exact power-one grid, tag preservation, four guards, all-power winding/area\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
