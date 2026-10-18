#include "source_displacement_core_geometry_candidate.h"
#include <iostream>
#include <limits>
int main(){try{
 std::array<std::array<float,3>,4>corners{{{0,0,0},{0,8,0},{8,8,0},{8,0,0}}};std::vector<std::array<float,5>>raw(9,{0,0,1,2,7});std::vector<uint16_t>tags(8,0x1234);
 auto r=sourceDisplacementCoreGeometryCandidate(1,1,0x80000008,corners,corners[2],raw,tags);
 if(r.surfaceFlags!=8||r.contents!=1||r.positions.front()!=std::array<float,3>{8,8,2}||r.positions.back()!=std::array<float,3>{0,0,2}||r.alphas!=std::vector<float>(9,7)||r.triangles.front().vertices!=std::array<uint16_t,3>{0,3,4}||r.triangles.front().tags!=0x1234)throw std::runtime_error("analytical composed collision geometry");
 unsigned rejected=0;auto reject=[&](auto f){try{f();}catch(const std::runtime_error&){++rejected;return;}throw std::runtime_error("invalid geometry input accepted");};
 reject([&]{sourceDisplacementCoreGeometryCandidate(5,1,0,corners,corners[0],raw,tags);});tags.pop_back();reject([&]{sourceDisplacementCoreGeometryCandidate(1,1,0,corners,corners[0],raw,tags);});tags.resize(8);raw.pop_back();reject([&]{sourceDisplacementCoreGeometryCandidate(1,1,0,corners,corners[0],raw,tags);});raw.resize(9);raw[0][3]=std::numeric_limits<float>::quiet_NaN();reject([&]{sourceDisplacementCoreGeometryCandidate(1,1,0,corners,corners[0],raw,tags);});raw[0][3]=0;reject([&]{sourceDisplacementCoreGeometryCandidate(1,1,0,corners,{100000,100000,100000},raw,tags);});corners[1][0]=std::numeric_limits<float>::infinity();reject([&]{sourceDisplacementCoreGeometryCandidate(1,1,0,corners,corners[0],raw,tags);});if(rejected!=6)throw std::runtime_error("guard count");
 std::cout<<"PASS composed orientation/position/alpha/flags/triangle fixture and six guards\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
