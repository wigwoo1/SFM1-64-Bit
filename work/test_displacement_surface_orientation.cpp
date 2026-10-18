#include "source_displacement_surface_orientation_candidate.h"
#include <iostream>
#include <limits>
int main(){try{
 SourceDisplacementSurfaceOrientationCandidate s;s.corners={{{0,0,0},{8,0,0},{8,8,0},{0,8,0}}};s.start={8,8,0};if(sourceDisplacementStartCornerCandidate(s)!=2)throw std::runtime_error("exact nearest corner");
 s.start={4,4,0};if(sourceDisplacementStartCornerCandidate(s)!=0)throw std::runtime_error("first distance tie");s.startCorner=3;if(sourceDisplacementStartCornerCandidate(s)!=3)throw std::runtime_error("cached index ignored");
 for(size_t i=0;i<4;++i){s.secondaryVectors[i].fill(float(10+i));s.pairs[i].fill(float(20+i));s.unrotatedScalars[i]=float(30+i);s.attributes0[i].fill(float(40+i));s.attributes1[i].fill(float(50+i));for(auto&v:s.vectors[i])v.fill(float(60+i));}
 const auto before=s;sourceDisplacementRotateSurfaceCandidate(s);
 for(size_t i=0;i<4;++i){const auto j=(i+3)%4;if(s.corners[i]!=before.corners[j]||s.secondaryVectors[i]!=before.secondaryVectors[j]||s.pairs[i]!=before.pairs[j]||s.attributes0[i]!=before.attributes0[j]||s.attributes1[i]!=before.attributes1[j]||s.vectors[i]!=before.vectors[j])throw std::runtime_error("auxiliary rotation");}if(s.unrotatedScalars!=before.unrotatedScalars||s.startCorner!=3||s.start!=before.start)throw std::runtime_error("unrotated fields changed");
 s={};s.start={10000,30000,0};if(sourceDisplacementStartCornerCandidate(s)!=-1)throw std::runtime_error("exact 1e9 cutoff accepted");s.start={10000,29999,0};if(sourceDisplacementStartCornerCandidate(s)!=0)throw std::runtime_error("below cutoff rejected");
 unsigned rejected=0;auto reject=[&](auto run){try{run();}catch(const std::runtime_error&){++rejected;return;}throw std::runtime_error("invalid input accepted");};
 s.startCorner=4;reject([&]{sourceDisplacementStartCornerCandidate(s);});s.startCorner=-1;s.start[0]=std::numeric_limits<float>::infinity();reject([&]{sourceDisplacementStartCornerCandidate(s);});s.start={0,0,0};s.corners[0][0]=std::numeric_limits<float>::quiet_NaN();reject([&]{sourceDisplacementStartCornerCandidate(s);});s.corners[0][0]=std::numeric_limits<float>::max();reject([&]{sourceDisplacementStartCornerCandidate(s);});s={};const auto saved=s;reject([&]{sourceDisplacementRotateSurfaceCandidate(s);});if(s.corners!=saved.corners||s.startCorner!=saved.startCorner||rejected!=5)throw std::runtime_error("rotation guard changed state");
 std::cout<<"PASS nearest/tie/cache/cutoff, six rotated groups, retained scalar group and five guards\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
