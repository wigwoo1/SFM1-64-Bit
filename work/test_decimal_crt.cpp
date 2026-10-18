#include "decimal_crt.h"
#include <iostream>
#include <cmath>
int main(){struct Case{const char*text;uint64_t bits;size_t end;int error;};
 const Case cases[]={
 {"x",0,0,7},{" \t-x",0,0,7},{" \v\f1.5tail",0x3ff8000000000000ull,6,7},
 {"-0",0x8000000000000000ull,2,7},{"1e99999",0x7ff0000000000000ull,7,34},
 {" -1e99999",0xfff0000000000000ull,9,34},
 {"1e-99999",0,8,34},{"-1e-99999",0,9,34},
 {"1e-310",0x000012688b70e62bull,6,7},{"1e+",0x3ff0000000000000ull,1,7}
 };unsigned count=0;
 for(const auto&c:cases){auto actual=nativeDecimalCrt(c.text,7);if(actual.bits!=c.bits||actual.end!=c.end||actual.error!=c.error){std::cerr<<c.text<<" end "<<actual.end<<"\n";return 1;}++count;}
 size_t cursor=2;float value=9;int error=7;
 if(!expressionNumber("xx 1.25tail",cursor,value,error)||cursor!=7||value!=1.25f||error!=7)return 2;++count;
 cursor=0;if(!expressionNumber("1e40",cursor,value,error)||cursor!=4||!std::isinf(value)||error!=7)return 3;++count;
 cursor=0;value=9;if(expressionNumber("x",cursor,value,error)||cursor||value!=9)return 4;++count;
 std::cout<<count<<" CRT and expression numeric integration checks passed\n";
}
