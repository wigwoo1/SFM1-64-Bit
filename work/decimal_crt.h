#pragma once
#include "decimal_conversion.h"
#include <cstring>
struct DecimalCrtResult {uint64_t bits;size_t end;int error;};
inline DecimalCrtResult nativeDecimalCrt(std::string_view text,int previousError=0){
 size_t start=0;while(start<text.size()&&(text[start]==' '||text[start]=='\t'||text[start]=='\r'||text[start]=='\n'||text[start]=='\v'||text[start]=='\f'))++start;
 auto conversion=nativeDecimalConversion(text.substr(start));
 if(conversion.flags&0x240)return {0,0,previousError};
 uint64_t bits=conversion.bits;int error=previousError;
 if(conversion.flags&0x81){bits=0x7ff0000000000000ull;if(start<text.size()&&text[start]=='-')bits|=0x8000000000000000ull;error=34;}
 else if((conversion.flags&0x100)&&(bits&0x7fffffffffffffffull)==0){error=34;bits=0;}
 return {bits,start+conversion.consumed,error};
}
// Expression primary stores the CRT binary64 result to binary32 after
// deciding success from end-pointer movement, not from errno.
inline bool expressionNumber(std::string_view text,size_t&cursor,float&value,int&error){
 if(cursor>text.size())return false;
 auto result=nativeDecimalCrt(text.substr(cursor),error);error=result.error;
 if(result.end==0)return false;
 double binary64;std::memcpy(&binary64,&result.bits,8);value=static_cast<float>(binary64);cursor+=result.end;return true;
}
