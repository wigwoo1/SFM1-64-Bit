#include "time_scale.h"
#include "time_selection.h"
#include <iostream>
int main(){struct Case{int32_t ticks;float scale;int32_t expected;};Case cases[]={{0,1,0},{80000,1,80000},{80000,0.5f,40000},{-80000,0.5f,-40000},{3,0.5f,2},{-3,0.5f,-1},{1,-1,-1}};
 for(auto c:cases)if(scaleTicks(c.ticks,c.scale)!=c.expected){std::cerr<<"scaling mismatch\n";return 1;}
 if(selectionToChild({10,20,30,40},10,5,2,false)!=std::array<int32_t,4>{10,30,50,70})return 2;
 if(selectionToChild({0,0,0,0},0,0,1,true)!=std::array<int32_t,4>{-1,0,-1,0})return 3;
 if(selectionToChild({-2147483647,0,0,2147483647},0,0,1,true)!=std::array<int32_t,4>{-2147483647,-2147483647,2147483647,2147483647})return 4;
 if(timeToChild(20,10,100,5,2,false)!=30)return 5;
 if(timeToChild(0,10,100,5,2,true)!=10)return 6;
 if(timeToChild(200,10,100,5,2,true)!=210)return 7;
 if(timeToChild(0,10,100,5,2,false)!=-10)return 8;
 std::cout<<"7 scaling, 3 selection and 4 scalar-transform cases passed\n";}
