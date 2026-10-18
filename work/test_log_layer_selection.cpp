#include "log_layer_selection.h"
#include <iostream>
#include <stdexcept>
int main(){try{
 auto check=[](const std::vector<LogLayerBounds>&l,int32_t t,int32_t wanted){if(selectLogLayer(l,t)!=wanted)throw std::runtime_error("layer selection mismatch");};
 check({},0,-1);std::vector<LogLayerBounds> l={{false,false,{0,100}},{false,false,{20,30}}};
 check(l,20,1);check(l,30,1);check(l,31,0);check(l,1000,0);
 check({{false,false,{0,100}},{false,false,{INT32_MIN,30}}},25,0);
 check({{false,false,{0,100}},{true,true,{}}},-2147483647,1);
 check({{false,false,{0,100}},{true,true,{}}},INT32_MIN,0);
 std::cout<<"8 layer-selection cases passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
