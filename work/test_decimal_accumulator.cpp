#include "decimal_accumulator.h"
#include <iostream>
int main(){struct C{const char*s;std::array<uint32_t,3> expected;};
 C cases[]={{"1",{0,0,0x3fff8000u}},{"10",{0,0,0x4002a000u}},{"4294967296",{0,0,0x401f8000u}},{"18446744073709551616",{0,0,0x403f8000u}},{"999999999999999999999999",{0xa0ffffffu,0x1bceccedu,0x404ed3c2u}}};
 for(auto&c:cases)if(decimalExtendedInteger(c.s)!=c.expected)return 1;
 for(auto s:{"","0","x","1234567890123456789012345"}){bool rejected=false;try{decimalExtendedInteger(s);}catch(const std::runtime_error&){rejected=true;}if(!rejected)return 2;}
 std::cout<<"5 extended integer fixtures and 4 domain checks passed\n";
}
