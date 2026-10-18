#include "source_displacement_grid_positions_candidate.h"
#include <functional>
#include <iostream>
#include <limits>
int main(){try{
 const std::array<SourceDisplacementGridPointCandidate,4>corners={{{0,0,0},{0,8,0},{8,8,0},{8,0,0}}};
 std::vector<SourceDisplacementGridOffsetCandidate>offsets(25);for(auto&v:offsets){v.direction={0,0,1};v.distance=2;}
 const auto grid=sourceDisplacementGridPositionsCandidate(2,corners,offsets);
 for(size_t y=0;y<5;++y)for(size_t x=0;x<5;++x)if(grid[y*5+x]!=SourceDisplacementGridPointCandidate{float(x*2),float(y*2),2})throw std::runtime_error("analytical plane grid");
 unsigned rejected=0;auto rejects=[&](const std::function<void()>&call){try{call();}catch(const std::runtime_error&){++rejected;return;}throw std::runtime_error("invalid input accepted");};
 rejects([&]{sourceDisplacementGridPositionsCandidate(0,corners,offsets);});
 rejects([&]{sourceDisplacementGridPositionsCandidate(32,corners,offsets);});
 rejects([&]{sourceDisplacementGridPositionsCandidate(1,corners,offsets);});
 auto badCorners=corners;badCorners[0][0]=std::numeric_limits<float>::infinity();
 rejects([&]{sourceDisplacementGridPositionsCandidate(2,badCorners,offsets);});
 auto badOffsets=offsets;badOffsets[0].direction[0]=std::numeric_limits<float>::quiet_NaN();
 rejects([&]{sourceDisplacementGridPositionsCandidate(2,corners,badOffsets);});
 badOffsets=offsets;badOffsets[0].distance=std::numeric_limits<float>::infinity();
 rejects([&]{sourceDisplacementGridPositionsCandidate(2,corners,badOffsets);});
 badOffsets=offsets;badOffsets[0].direction[0]=std::numeric_limits<float>::max();badOffsets[0].distance=2;
 rejects([&]{sourceDisplacementGridPositionsCandidate(2,corners,badOffsets);});
 if(rejected!=7||offsets[0].distance!=2||corners[0][0]!=0)throw std::runtime_error("guard coverage or input mutation");
 std::cout<<"PASS analytical plane 25 points, seven invalid-input rejections\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
