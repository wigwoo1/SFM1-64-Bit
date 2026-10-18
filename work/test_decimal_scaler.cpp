#include "decimal_scaler.h"
#include <iostream>
struct Fixture{NormalizedExtended input;int exponent;NormalizedExtended expected;};
int main(){Fixture fixtures[]={
#include "decimal_scaler_fixtures.inc"
};size_t n=0;for(auto&f:fixtures){auto actual=scaleDecimalExtended(f.input,f.exponent);if(actual.exponent!=f.expected.exponent||actual.significand!=f.expected.significand){std::cerr<<"Mismatch "<<n<<" exponent "<<f.exponent<<'\n';return 1;}++n;}std::cout<<n<<" composed decimal scaler fixtures passed\n";}
