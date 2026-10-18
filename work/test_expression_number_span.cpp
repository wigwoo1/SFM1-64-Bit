#include "expression_number_span.h"
#include <iostream>
int main(){struct Case{const char*text;bool ok;size_t end;};
 Case cases[]={{"12.5x",true,4},{".5",true,2},{"1.",true,2},{"-0",true,2},{"+",false,0},{".",false,0},{"1e",true,1},{"1e+",true,1},{"1E-3",true,4},{"2D+4",true,4},{"3d2",true,3},{"1-2",true,1},{"\n 2",true,3},{"0x12",true,1},{"nan",false,0},{"inf",false,0},{"  nope",false,0}};
 for(auto c:cases){size_t p=0;if(expressionNumberSpan(c.text,p)!=c.ok||p!=c.end){std::cerr<<c.text<<'\n';return 1;}}
 std::cout<<"17 decimal scanner boundary cases passed\n";
}
