#include "decimal_crt.h"
#include <fstream>
#include <iostream>
int main(){static_assert(sizeof(void*)==8,"x64 replay required");
 std::ifstream fixtures("work/original_expression_float_fixtures.bin",std::ios::binary);if(!fixtures)return 1;
 unsigned length,count=0;
 while(fixtures.read(reinterpret_cast<char*>(&length),4)){
  if(length>1024)return 2;std::string text(length,'\0');unsigned consumed;int expectedError;uint32_t expectedBits;
  if(!fixtures.read(text.data(),length)||!fixtures.read(reinterpret_cast<char*>(&consumed),4)||!fixtures.read(reinterpret_cast<char*>(&expectedError),4)||!fixtures.read(reinterpret_cast<char*>(&expectedBits),4))return 3;
  size_t cursor=0;float value=9;int error=7;bool success=expressionNumber(text,cursor,value,error);uint32_t bits;std::memcpy(&bits,&value,4);
  if(success!=(consumed!=0)||cursor!=consumed||error!=expectedError||bits!=expectedBits)return 4;++count;
 }
 if(!fixtures.eof()||fixtures.gcount()!=0||count!=40031)return 5;
 std::cout<<count<<" original expression float fixtures passed in x64 executable\n";
}
