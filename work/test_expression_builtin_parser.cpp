#include "expression_parser.h"
#include <iostream>
int main(){ExpressionVariables variables;
 struct Case{const char*text;float expected;};const Case cases[]={
 {"abs(-3)",3},{"sqr(3)",9},{"sqrt(9)",3},{"dtor(0)",0},{"rtod(0)",0},
 {"floor(-1.25)",-2},{"round(-1.5)",-1},{"sign(0)",1},{"min(2,3)",2},{"max(2,3)",3},
 {"inrange(2,1,3)",1},{"clamp(5,1,3)",3},{"ramp(2,0,4)",0.5f},{"lerp(.5,2,4)",3},
 {"cramp(8,0,4)",1},{"clerp(2,2,4)",4},{"elerp(.5,2,4)",3},
 {"rescale(5,0,10,0,100)",50},{"crescale(20,0,10,0,100)",100},
 {"MAX(abs(-2),sqr(3))+sqrt(4)",11},{"lerp(1?0.5:0,2+2,8)",6},
 {"1+clamp(2,0,",1}
 };unsigned count=0;
 for(const auto&c:cases){ExpressionParser parser(c.text,variables);float value;if(!parser.evaluate(value)||value!=c.expected){std::cerr<<c.text<<"\n";return 1;}++count;}
 for(auto text:{"clamp(2,0)","max(1,2,3)","abs()","sqrt(4","noise(1,2,3)"}){ExpressionParser parser(text,variables);float value=9;if(parser.evaluate(value)||value!=0||parser.consumed()!=0)return 2;++count;}
 variables.set("max",7);ExpressionParser variableFirst("max(1,2)",variables);float value;if(!variableFirst.evaluate(value)||value!=7||variableFirst.consumed()!=3)return 3;++count;
 std::cout<<count<<" builtin expression parser cases passed\n";
}
