#include "decimal_scan.h"
#include <iostream>
int main(){struct Case{const char*text;const char*digits;int scale;bool negative;size_t end;};
 const Case cases[]={
 {"1","1",0,false,1},{"00120","12",1,false,5},{".00120","12",-4,false,6},
 {"-1.25d+2","125",0,true,8},{"1e+","1",0,false,1},{"0.000","",-3,false,5},
 {"1e999999999999","1",5201,false,14},{"1e-999999999999","1",-5201,false,15},
 {"1234567890123456789012349","123456789012345678901234",1,false,25},
 {"1234567890123456789012350","123456789012345678901236",1,false,25},
 {"1234567890123456789012390","12345678901234567890123:",1,false,25},
 {"1.234567890123456789012359","123456789012345678901236",-23,false,26}
 };unsigned count=0;
 for(const auto&c:cases){auto result=scanNativeDecimal(c.text);std::string ascii;
  if(result)for(char value:result->numericDigits)ascii.push_back(char('0'+value));
  if(!result||ascii!=c.digits||result->scale!=c.scale||result->negative!=c.negative||result->end!=c.end){std::cerr<<c.text<<"\n";return 1;}++count;
 }
 for(const auto&text:{"",".","-.","e10","x"}){if(scanNativeDecimal(text))return 2;++count;}
 std::cout<<count<<" native decimal retention cases passed\n";
}
