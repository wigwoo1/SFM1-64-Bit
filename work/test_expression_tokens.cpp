#include "expression_tokens.h"
#include <iostream>
int main(){
 size_t p=0;if(!expressionToken(" \tSinExtra",p,"sin")||p!=5)return 1;
 p=0;if(expressionToken(" \tcos",p,"sin")||p!=0)return 2;
 p=0;if(expressionToken("\nsin",p,"sin")||p!=0)return 3;
 ExpressionVariables vars;vars.set("x",1);vars.set("xy",2);
 float result=-1;p=0;if(!vars.match("XYmore",p,result)||p!=2||result!=2)return 4;
 vars.set("x",3);p=0;if(vars.size()!=2||!vars.match("xy",p,result)||result!=2)return 5;
 vars.set("X",4);p=0;if(vars.size()!=3||!vars.match("xy",p,result)||p!=1||result!=4)return 6;
 p=1;result=7;if(vars.match("?none",p,result)||p!=1||result!=7)return 7;
 std::cout<<"7 token/variable integration cases passed\n";
}
