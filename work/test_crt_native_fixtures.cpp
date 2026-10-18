#include "decimal_crt.h"
#include <fstream>
#include <iostream>
int main(){static_assert(sizeof(void*)==8,"x64 replay required");
 std::ifstream fixtures("work/original_crt_fixtures.bin",std::ios::binary);if(!fixtures)return 1;
 unsigned length,count=0;
 while(fixtures.read(reinterpret_cast<char*>(&length),4)){
  if(length>1024)return 2;std::string text(length,'\0');unsigned consumed,flags;uint64_t bits;
  if(!fixtures.read(text.data(),length)||!fixtures.read(reinterpret_cast<char*>(&consumed),4)||!fixtures.read(reinterpret_cast<char*>(&flags),4)||!fixtures.read(reinterpret_cast<char*>(&bits),8))return 3;
  auto actual=nativeDecimalCrt(text,7);if(actual.end!=consumed||unsigned(actual.error)!=flags||actual.bits!=bits)return 4;++count;
 }
 if(!fixtures.eof()||fixtures.gcount()!=0||count!=20025)return 5;
 std::cout<<count<<" original outer CRT fixtures passed in x64 executable\n";
}
