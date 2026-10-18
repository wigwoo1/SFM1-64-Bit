#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <cstdint>
struct DecimalScan {std::string numericDigits;int64_t scale;bool negative;size_t end;};
// Default decimal separator; wrapper whitespace classification uses C locale.
inline std::optional<DecimalScan> scanNativeDecimal(std::string_view text,size_t start=0,bool wrapperWhitespace=true){
 if(start>text.size())return std::nullopt;
 size_t p=start;while(p<text.size()&&(text[p]==' '||text[p]=='\t'||text[p]=='\r'||text[p]=='\n'||(wrapperWhitespace&&(text[p]=='\v'||text[p]=='\f'))))++p;
 bool negative=false;if(p<text.size()&&(text[p]=='+'||text[p]=='-'))negative=text[p++]=='-';
 std::string digits;int64_t displacement=0;bool any=false,significant=false;
 auto digit=[](char c){return c>='0'&&c<='9';};
 while(p<text.size()&&digit(text[p])){
  char value=char(text[p++]-'0');any=true;
  if(!significant&&value==0)continue;significant=true;
  if(digits.size()<25)digits.push_back(value);else ++displacement;
 }
 if(p<text.size()&&text[p]=='.'){
  ++p;while(p<text.size()&&digit(text[p])){
   char value=char(text[p++]-'0');any=true;
   if(!significant&&value==0){--displacement;continue;}significant=true;
   if(digits.size()<25){digits.push_back(value);--displacement;}
  }
 }
 if(!any)return std::nullopt;
 const size_t mantissaEnd=p;int exponent=0;bool exponentNegative=false;
 if(p<text.size()&&(text[p]=='e'||text[p]=='E'||text[p]=='d'||text[p]=='D')){
  ++p;if(p<text.size()&&(text[p]=='+'||text[p]=='-'))exponentNegative=text[p++]=='-';
  const size_t exponentStart=p;
  while(p<text.size()&&digit(text[p])){exponent=exponent<=5200?exponent*10+(text[p]-'0'):5201;if(exponent>5200)exponent=5201;++p;}
  if(p==exponentStart){p=mantissaEnd;exponent=0;}
 }
 // Native 0x10A1E15C reads local -9 (the 24th digit), not the 25th.
 // Increment is byte-local; a 9 can become numeric 10 without propagation.
 if(digits.size()>24){if(static_cast<unsigned char>(digits[23])>=5)++digits[23];digits.resize(24);++displacement;}
 while(!digits.empty()&&digits.back()==0){digits.pop_back();++displacement;}
 displacement+=exponentNegative?-exponent:exponent;
 return DecimalScan{digits,displacement,negative,p};
}
