#include "extended_normalize.h"
#include <iostream>
int main(){
 struct Fixture{ExtendedProduct input;int exponent;NormalizedExtended expected;};
 Fixture fixtures[]={
#include "extended_normalize_fixtures.inc"
 };
 for(auto&f:fixtures){auto actual=normalizeExtendedProduct(f.input,f.exponent);if(actual.exponent!=f.expected.exponent||actual.significand!=f.expected.significand)return 10;}
 auto a=normalizeExtendedProduct({0,0,0,0,0,0x8000},16383);if(a.exponent!=16383||a.significand!=ExtendedLimbs{0,0,0,0,0x8000})return 1;
 auto b=normalizeExtendedProduct({0,0,0,0,0,0x4000},16383);if(b.exponent!=16382||b.significand[4]!=0x8000)return 2;
 auto c=normalizeExtendedProduct({0x8000,2,0,0,0,0x8000},16383);if(c.significand[0]!=2)return 3;
 auto d=normalizeExtendedProduct({0x8000,3,0,0,0,0x8000},16383);if(d.significand[0]!=4)return 4;
 auto e=normalizeExtendedProduct({0x8001,0xffff,0xffff,0xffff,0xffff,0xffff},16383);if(e.exponent!=16384||e.significand!=ExtendedLimbs{0,0,0,0,0x8000})return 5;
 auto f=normalizeExtendedProduct({0,0,0,0,0,0x8000},0);if(f.exponent!=0||f.significand[4]!=0x4000)return 6;
 auto g=normalizeExtendedProduct({1,1,0,0,0,0x8000},0);if(g.significand[0]!=1)return 7;
 auto h=normalizeExtendedProduct({0,0,0,0,0,0x8000},-100);if(h.exponent||h.significand!=ExtendedLimbs{})return 8;
 auto i=normalizeExtendedProduct({0x8001,0xffff,0xffff,0xffff,0xffff,0xffff},32766);if(i.exponent!=32767||i.significand[4]!=0x8000)return 9;
 std::cout<<"9 focused and 2000 arbitrary-precision normalization cases passed\n";
}
