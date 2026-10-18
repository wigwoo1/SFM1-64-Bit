#include "expression_parser.h"
#include <random>
#include <cmath>
#include <iostream>
int main(){std::mt19937 random(10010);unsigned count=0;
 for(unsigned i=0;i<100000;++i){uint32_t bits=random();float value;std::memcpy(&value,&bits,4);if(std::isnan(value))continue;
  auto actual=expressionCeiling(value);float expected=float(std::ceil(double(value)));uint32_t a,b;std::memcpy(&a,&*actual,4);std::memcpy(&b,&expected,4);if(a!=b)return 1;++count;
 }
 ExpressionVariables variables;
 for(auto text:{"ceiling(-1.25)","ceiling(-0.25)","ceiling(1.25)"}){ExpressionParser parser(text,variables);float value;if(!parser.evaluate(value)||value!=float(std::ceil(text[8]=='-'?(text[9]=='1'?-1.25:-0.25):1.25)))return 2;++count;}
 float nan=std::nanf("");if(expressionCeiling(nan))return 3;++count;
 std::cout<<count<<" ceiling finite/infinity and integration checks passed\n";
}
