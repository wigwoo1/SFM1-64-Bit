#include "ik_prepared_solution.h"
#include <iostream>
int main(){const auto s=recoveredIkPreparedSolutionCandidate({0,0,0},{1,0,0},{0,0,1},1,1,{0,0,0,1},{0,0,0,1});
 for(auto q:{s.start,s.mid})for(float v:q)if(!std::isfinite(v))return 1;
 std::cout<<"prepared_solution_smoke=finite"<<std::endl;return 0;}
