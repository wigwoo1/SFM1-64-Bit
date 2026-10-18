#include "decimal_value.h"
#include <iostream>
int main(){
 struct Case{const char*digits;int scale;uint64_t expected;unsigned status;};
 const Case cases[]={
 {"0",0,0,0},{"000",-5000,0,0},{"1",0,0x3ff0000000000000ull,0},
 {"3",0,0x4008000000000000ull,0},{"125",-3,0x3fc0000000000000ull,0},
 {"15",-1,0x3ff8000000000000ull,0},{"25",-1,0x4004000000000000ull,0},
 {"1",1,0x4024000000000000ull,0},{"1",2,0x4059000000000000ull,0},
 {"1",3,0x408f400000000000ull,0},{"4294967296",0,0x41f0000000000000ull,0},
 {"9007199254740992",0,0x4340000000000000ull,0},
 {"1",5200,0x7ff0000000000000ull,1},{"1",-5200,0,0}
 };
 unsigned count=0;
 for(const auto&c:cases)for(bool negative:{false,true}){
  auto actual=convertRetainedDecimal(c.digits,c.scale,negative);
  if(actual.bits!=(c.expected|(negative?0x8000000000000000ull:0ull))||actual.status!=c.status){std::cerr<<c.digits<<" scale "<<c.scale<<" status "<<actual.status<<"\n";return 1;}++count;
 }
 for(const auto&digits:{"","x","1234567890123456789012345"}){try{convertRetainedDecimal(digits,0);return 2;}catch(const std::runtime_error&){}++count;}
 for(int exponent:{-5201,5201}){try{convertRetainedDecimal("1",exponent);return 3;}catch(const std::runtime_error&){}++count;}
 if(retainedDecimalDouble(convertRetainedDecimal("125",-3))!=0.125)return 4;
 std::cout<<count<<" retained-decimal integration cases passed\n";
}
