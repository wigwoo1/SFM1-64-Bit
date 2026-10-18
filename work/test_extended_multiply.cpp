#include "extended_multiply.h"
#include "decimal_accumulator.h"
#include "decimal_scaler.h"
#include <iostream>
NormalizedExtended integer(const char*s){auto v=decimalExtendedInteger(s);return {{uint16_t(v[0]),uint16_t(v[0]>>16),uint16_t(v[1]),uint16_t(v[1]>>16),uint16_t(v[2])},uint16_t(v[2]>>16)};}
int main(){struct C{const char*a,*b,*answer;};C cases[]={{"1","1","1"},{"10","10","100"},{"2","3","6"},{"4294967296","4294967296","18446744073709551616"}};
 for(auto&c:cases){auto v=multiplyExtended(integer(c.a),integer(c.b)),e=integer(c.answer);if(v.significand!=e.significand||v.exponent!=e.exponent)return 1;}
 auto negative=integer("2");negative.exponent|=0x8000;auto v=multiplyExtended(negative,integer("3")),e=integer("6");if(v.significand!=e.significand||v.exponent!=(e.exponent|0x8000))return 2;
 if(multiplyExtended({},integer("1")).exponent)return 3;
 auto large=integer("1");large.exponent=0x7ffe;if(multiplyExtended(large,large).exponent!=0x7fff)return 4;
 auto tiny=integer("1");tiny.exponent=1;if(multiplyExtended(tiny,tiny).exponent)return 5;
 for(auto c:{C{"1","1","10"},C{"2","1","20"},C{"3","1","30"}}){auto scaled=scaleDecimalExtended(integer(c.a),1),expected=integer(c.answer);if(scaled.exponent!=expected.exponent||scaled.significand!=expected.significand)return 6;}
 auto unchanged=scaleDecimalExtended(integer("3"),0);if(unchanged.significand!=integer("3").significand)return 7;
 std::cout<<"8 composed multiplication and 4 decimal scaling cases passed\n";
}
