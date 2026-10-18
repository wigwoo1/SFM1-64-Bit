#include "float_interpolation.h"
#include <iostream>
#include <stdexcept>
int main(){try{
 if(timeFraction(5,0,10,false)!=0.5f||timeFraction(5,5,5,false)!=0||timeFraction(-5,0,10,true)!=0||timeFraction(15,0,10,true)!=1||timeFraction(15,0,10,false)!=1.5f)throw std::runtime_error("fraction mismatch");
 if(interpolateFloat(-4,8,0.5f)!=2||interpolateFloat(-4,8,0)!=-4||interpolateFloat(-4,8,1)!=8)throw std::runtime_error("interpolation mismatch");
 std::cout<<"8 float interpolation cases passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
