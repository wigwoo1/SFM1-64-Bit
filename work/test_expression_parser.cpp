#include "expression_parser.h"
#include <iostream>
#include <cmath>
int main(){ExpressionVariables variables;variables.set("x",4);variables.set("xx",6);
 struct Case{const char*text;float value;size_t consumed;};
 const Case cases[]={
 {"1+2*3",7,5},{"10-3-2",9,6},{"16/4/2",8,6},{"(10-3)-2",5,8},
 {"x+xx",10,4},{"XXtail",6,2},{"!0+--2",3,6},{"1<2==1",1,6},
 {"1||0&&0",1,7},{"0?2:3",3,5},{"1?0?2:3:4",3,9},
 {"1+",1,1},{"1?2",1,1},{"1garbage",1,1},{"1+2e+",3,3},
 {" \v1.25",1.25f,6},{"0&&(1e99999)",0,12}
 };unsigned count=0;
 for(const auto&c:cases){ExpressionParser parser(c.text,variables);float result=9;if(!parser.evaluate(result)||result!=c.value||parser.consumed()!=c.consumed){std::cerr<<c.text<<" value "<<result<<" cursor "<<parser.consumed()<<"\n";return 1;}++count;}
 for(auto text:{"","(","(1+2","unknown","sin(1)"}){ExpressionParser parser(text,variables);float result=9;if(parser.evaluate(result)||result!=0||parser.consumed()!=0)return 2;++count;}
 ExpressionParser eager("0&&(1e99999)",variables);float result;if(!eager.evaluate(result)||eager.error()!=34)return 3;++count;
 std::cout<<count<<" integrated expression grammar checks passed\n";
}
