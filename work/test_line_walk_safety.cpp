#include "source_bsp_line_walk_candidate.h"
#include <iostream>
#include <limits>
int main(){try{
 for(unsigned test=0;test<4;++test){std::vector<SourceLineWalkPlaneCandidate>p{{{1,0,0},0,0}};std::vector<SourceLineWalkNodeCandidate>n{{0,{-1,-2}}};std::array<float,3>start{-1,0,0},end{1,0,0};float hit=1;unsigned calls=0;
  if(test==0)n[0].children[0]=0;if(test==1)n[0].children[1]=99;if(test==2)n[0].plane=5;if(test==3)start[0]=std::numeric_limits<float>::infinity();bool rejected=false;
  try{sourceBspLineWalkCandidate(p,n,0,2,start,end,0,1,hit,[&](uint32_t,float,float){++calls;});}catch(const std::runtime_error&){rejected=true;}
  if(!rejected||calls||hit!=1)throw std::runtime_error("invalid tree reached callback or changed hit");
 }
 std::cout<<"PASS four malformed inputs rejected before callbacks\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
