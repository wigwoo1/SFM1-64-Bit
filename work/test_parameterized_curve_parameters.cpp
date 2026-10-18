#include "parameterized_curve_parameters.h"
#include <iostream>
int main(){static_assert(sizeof(void*)==8,"x64 required");for(uint32_t code=0;code<256;++code){auto p=recoveredParameterizedCurve(code);if(code>=7&&code<=9){if(p.first!=0.77f||p.third!=0.77f||p.second!=(code==7?0.f:code==8?-1.f:1.f))return 1;}else if(p.first||p.second||p.third)return 1;}std::cout<<"parameterized_curve_codes=256 mappings=verified\n";}
