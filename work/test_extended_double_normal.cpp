#include "extended_double_normal.h"
#include <iostream>
int main(){
 struct Case {NormalizedExtended value;uint64_t bits;unsigned status;};
 const Case cases[]={
  {{{0,0,0,0,0x8000},0x3fff},0x3ff0000000000000ull,0},
  {{{0,0,0,0,0xc000},0x4000},0x4008000000000000ull,0},
  {{{0,0,0,0,0x8000},0xbfff},0xbff0000000000000ull,0},
  {{{0,0,0,0,0x8000},0x3c01},0x0010000000000000ull,0},
  {{{0,0xf800,0xffff,0xffff,0xffff},0x43fe},0x7fefffffffffffffull,0},
  {{{1,0xfc00,0xffff,0xffff,0xffff},0x43fe},0x7ff0000000000000ull,1},
  {{{0,0,0,0,0x8000},0x43ff},0x7ff0000000000000ull,1},
  {{{0,0,0,0,0x8000},0xc3ff},0xfff0000000000000ull,1}
 };
 for(const auto&c:cases){auto actual=packExtendedDoubleNormal(c.value);if(!actual||actual->bits!=c.bits||actual->status!=c.status)return 1;}
 if(packExtendedDoubleNormal({{0,0,0,0,0x8000},0x3c00}))return 2;
 std::cout<<"8 normal/overflow packing cases and 1 unresolved-underflow check passed\n";
}
