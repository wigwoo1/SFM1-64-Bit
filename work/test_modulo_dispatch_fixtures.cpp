#include "expression_parser.h"
#include <fstream>
#include <iostream>
int main(){static_assert(sizeof(void*)==8,"x64 required");std::ifstream fixtures("outputs/sfm-modulo-dispatch-grid.txt");uint32_t a,b,expected;unsigned count=0;
 while(fixtures>>std::hex>>a>>b>>expected){float left,right;std::memcpy(&left,&a,4);std::memcpy(&right,&b,4);auto value=expressionModulo(left,right);uint32_t actual;std::memcpy(&actual,&*value,4);if(actual!=expected){std::cerr<<std::hex<<a<<" "<<b<<" expected "<<expected<<" actual "<<actual<<"\n";return 1;}++count;}
 if(!fixtures.eof()||count!=169)return 2;ExpressionVariables variables;
 for(auto text:{"10%4%3","10%3*2","(10%3)*2"}){ExpressionParser parser(text,variables);float result;float expectedValue=text[0]=='('?2:text[3]=='4'?0:4;if(!parser.evaluate(result)||result!=expectedValue)return 3;++count;}
 std::cout<<count<<" native modulo grid and parser cases passed\n";
}
