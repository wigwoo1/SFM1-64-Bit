#include "decimal_scanned_value.h"
#include <fstream>
#include <cstring>
#include <iostream>
int main(){static_assert(sizeof(void*)==8,"x64 replay required");
 std::ifstream fixtures("work/original_parser_fixtures.bin",std::ios::binary);if(!fixtures)return 1;
 unsigned length,count=0;
 while(fixtures.read(reinterpret_cast<char*>(&length),4)){
  if(length>1024)return 2;std::string text(length,'\0');unsigned consumed,flags;uint16_t expected[6];
  if(!fixtures.read(text.data(),length)||!fixtures.read(reinterpret_cast<char*>(&consumed),4)||!fixtures.read(reinterpret_cast<char*>(&flags),4)||!fixtures.read(reinterpret_cast<char*>(expected),12))return 3;
  auto actual=scannedDecimalExtended(text);
  if(actual.end!=consumed||actual.parserFlags!=flags||std::memcmp(actual.value.significand.data(),expected,10)||actual.value.exponent!=expected[5])return 4;++count;
 }
 if(!fixtures.eof()||fixtures.gcount()!=0||count!=20018)return 5;
 std::cout<<count<<" original parser fixtures passed in x64 executable\n";
}
