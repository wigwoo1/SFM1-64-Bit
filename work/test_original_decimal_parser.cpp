#include <windows.h>
#include <fstream>
#include <iostream>
#include <random>
#include <cstring>
#include <vector>
#include "decimal_scanned_value.h"
#include "original_decimal_parser.inc"
#include "original_decimal_accumulator.inc"
#include "original_scaler_kernel.inc"
#include "original_decimal_conversion.inc"
#include "original_double_packer.inc"
#include "decimal_conversion.h"
#include "decimal_crt.h"
#include "original_decimal_crt.inc"
void* suppliedLocale=nullptr;int suppliedError=0;
__declspec(naked) void suppliedLocaleInit(){__asm{
 mov eax,suppliedLocale
 mov [ecx],eax
 mov byte ptr [ecx+0ch],0
 ret 4
}}
int* __cdecl suppliedErrno(){return &suppliedError;}
__declspec(naked) void __cdecl originalFloat(const char*,const char**,float*){__asm{
 push ebp
 mov ebp,esp
 push [ebp+0ch]
 push [ebp+8]
 mov eax,10a103f1h
 call eax
 add esp,8
 mov eax,[ebp+10h]
 fstp dword ptr [eax]
 pop ebp
 ret
}}
void installJump(unsigned char*target,void*function){target[0]=0xb8;std::memcpy(target+1,&function,4);target[5]=0xff;target[6]=0xe0;}
int main(){
 auto memory=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x1000000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(memory!=reinterpret_cast<void*>(0x10000000))return 1;
 std::memcpy(memory+0xa1ded7,originalParser,sizeof originalParser);
 std::memcpy(memory+0xa1dd78,originalConversion,sizeof originalConversion);
 std::memcpy(memory+0xa102ce,originalCrt,sizeof originalCrt);
 installJump(memory+0xa0ee17,reinterpret_cast<void*>(&suppliedLocaleInit));
 installJump(memory+0xa16f8b,reinterpret_cast<void*>(&suppliedErrno));
 const uint64_t infinity=0x7ff0000000000000ull;std::memcpy(memory+0xf222a0,&infinity,8);
 std::memcpy(memory+0xa31655,originalCode,sizeof originalCode);
 std::memcpy(memory+0xa320f8,originalAccumulator,sizeof originalAccumulator);
 std::memcpy(memory+0xf22800,originalScaleTables,sizeof originalScaleTables);memory[0xa0e353]=0xc3;
 DWORD previous;if(!VirtualProtect(memory,0x1000000,PAGE_EXECUTE_READ,&previous))return 2;FlushInstructionCache(GetCurrentProcess(),memory,0x1000000);
 const char*decimal=".";const char**conversion=&decimal;unsigned char locale[256]{};void*localePointer=locale;std::memcpy(locale+0xbc,&conversion,4);
 uint16_t classifications[256]{};for(unsigned c:{9u,10u,11u,12u,13u,32u})classifications[c]=8;
 auto classificationPointer=classifications;std::memcpy(locale+0xc8,&classificationPointer,4);
 uint32_t singleByteLocale=1;std::memcpy(locale+0xac,&singleByteLocale,4);suppliedLocale=locale;
 using Original=unsigned(__cdecl*)(uint16_t*,const char**,const char*,unsigned,unsigned,unsigned,unsigned,void*);
 auto original=reinterpret_cast<Original>(memory+0xa1ded7);
 using Convert=void*(__cdecl*)(uint32_t*,const char*,void*);
 auto convert=reinterpret_cast<Convert>(memory+0xa1dd78);
 std::vector<std::string> cases={"",".","-.","x","0","-0","0.000","1e+","1d-","1e0","00120",".00120","-1.25d+2","1234567890123456789012390","1e999999999999","1e-999999999999"," "," +x"};
 std::mt19937 random(19078);
 for(unsigned i=0;i<20000;++i){std::string text=i%3==0?"-":"";if(i%7==0)text=" \t"+text;
  unsigned length=1+random()%65;for(unsigned n=0;n<length;++n){if(n==length/2&&i%2==0)text+='.';text+=char('0'+random()%10);}
  if(i%4==0){text+=(i%8==0?'D':'e');int exponent=int(random()%11001)-5500;text+=std::to_string(exponent);}
  text+="tail";cases.push_back(text);
 }
 std::ofstream fixtures("work/original_parser_fixtures.bin",std::ios::binary);if(!fixtures)return 3;unsigned count=0;
 std::ofstream conversionFixtures("work/original_conversion_fixtures.bin",std::ios::binary);if(!conversionFixtures)return 6;
 for(const auto&text:cases){uint16_t expected[6]{};const char*end=nullptr;unsigned flags=original(expected,&end,text.c_str(),0,0,0,0,&localePointer);auto actual=scannedDecimalExtended(text);
  unsigned consumed=unsigned(end-text.c_str());
  if(flags!=actual.parserFlags||consumed!=actual.end||std::memcmp(expected,actual.value.significand.data(),10)||expected[5]!=actual.value.exponent){std::cerr<<"Mismatch ["<<text<<"] end "<<consumed<<"/"<<actual.end<<" flags "<<flags<<"/"<<actual.parserFlags<<"\n";return 4;}
  unsigned length=unsigned(text.size());fixtures.write(reinterpret_cast<const char*>(&length),4);fixtures.write(text.data(),length);fixtures.write(reinterpret_cast<const char*>(&consumed),4);fixtures.write(reinterpret_cast<const char*>(&flags),4);fixtures.write(reinterpret_cast<const char*>(expected),12);++count;
  uint32_t converted[6]{};convert(converted,text.c_str(),&localePointer);
  uint64_t bits=uint64_t(converted[4])|(uint64_t(converted[5])<<32);
  const auto reconstructed=nativeDecimalConversion(text);
  if(reconstructed.bits!=bits||reconstructed.flags!=converted[0]||reconstructed.consumed!=converted[1]){std::cerr<<"Conversion mismatch ["<<text<<"]\n";return 7;}
  conversionFixtures.write(reinterpret_cast<const char*>(&length),4);conversionFixtures.write(text.data(),length);conversionFixtures.write(reinterpret_cast<const char*>(&converted[1]),4);conversionFixtures.write(reinterpret_cast<const char*>(&converted[0]),4);conversionFixtures.write(reinterpret_cast<const char*>(&bits),8);
 }
 fixtures.close();conversionFixtures.close();if(!fixtures||!conversionFixtures)return 5;
 using Crt=double(__cdecl*)(const char*,const char**,void*);
 auto crt=reinterpret_cast<Crt>(memory+0xa102ce);
 std::ofstream crtFixtures("work/original_crt_fixtures.bin",std::ios::binary);if(!crtFixtures)return 8;
 cases.insert(cases.end(),{"\v\f1.5tail"," \v\f-x","\v\f","1e-310","1e-324","-1e-324","1e40"});
 unsigned crtCount=0;
 for(const auto&text:cases){suppliedError=7;const char*end=nullptr;double result=crt(text.c_str(),&end,nullptr);uint64_t bits;std::memcpy(&bits,&result,8);
  const auto actual=nativeDecimalCrt(text,7);unsigned consumed=unsigned(end-text.c_str());
  if(actual.bits!=bits||actual.end!=consumed||actual.error!=suppliedError){std::cerr<<"CRT mismatch ["<<text<<"] error "<<suppliedError<<"/"<<actual.error<<"\n";return 9;}
  unsigned length=unsigned(text.size());crtFixtures.write(reinterpret_cast<const char*>(&length),4);crtFixtures.write(text.data(),length);crtFixtures.write(reinterpret_cast<const char*>(&consumed),4);crtFixtures.write(reinterpret_cast<const char*>(&suppliedError),4);crtFixtures.write(reinterpret_cast<const char*>(&bits),8);++crtCount;
 }
 crtFixtures.close();if(!crtFixtures)return 10;
 std::vector<std::string> floatCases=cases;
 floatCases.insert(floatCases.end(),{"1.000000059604644775390625","1.000000059604644775390626","16777217","1.401298464324817e-45","7.006492321624085e-46","3.4028235677973366e38"});
 for(unsigned i=0;i<20000;++i){std::string text=i%2?"-":"";text+=std::to_string(1+random()%1000000000);text+='e';text+=std::to_string(int(random()%121)-60);floatCases.push_back(text);}
 std::ofstream floatFixtures("work/original_expression_float_fixtures.bin",std::ios::binary);if(!floatFixtures)return 11;
 unsigned floatCount=0;
 for(const auto&text:floatCases){suppliedError=7;const char*end=nullptr;float originalValue=0;originalFloat(text.c_str(),&end,&originalValue);
  unsigned consumed=unsigned(end-text.c_str());size_t cursor=0;float actualValue=9;int error=7;bool success=expressionNumber(text,cursor,actualValue,error);
  uint32_t expectedBits,actualBits;float expectedValue=consumed?originalValue:9;std::memcpy(&expectedBits,&expectedValue,4);std::memcpy(&actualBits,&actualValue,4);
  if(success!=(consumed!=0)||cursor!=consumed||error!=suppliedError||actualBits!=expectedBits){std::cerr<<"Float mismatch ["<<text<<"]\n";return 12;}
  unsigned length=unsigned(text.size());floatFixtures.write(reinterpret_cast<const char*>(&length),4);floatFixtures.write(text.data(),length);floatFixtures.write(reinterpret_cast<const char*>(&consumed),4);floatFixtures.write(reinterpret_cast<const char*>(&suppliedError),4);floatFixtures.write(reinterpret_cast<const char*>(&expectedBits),4);++floatCount;
 }
 floatFixtures.close();VirtualFree(memory,0,MEM_RELEASE);if(!floatFixtures)return 13;
 std::cout<<count<<" original scanner/pipeline and binary64 wrapper differential cases passed\n"<<crtCount<<" outer CRT differential cases passed\n";
 std::cout<<floatCount<<" original x87 float-store expression cases passed\n";
}
