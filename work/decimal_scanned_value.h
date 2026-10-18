#pragma once
#include "decimal_scan.h"
#include "decimal_accumulator.h"
#include "decimal_scaler.h"
struct ScannedExtended {NormalizedExtended value;unsigned parserFlags;size_t end;};
inline ScannedExtended scannedDecimalExtended(std::string_view text){
 auto scanned=scanNativeDecimal(text,0,false);
 if(!scanned){size_t p=0;while(p<text.size()&&(text[p]==' '||text[p]=='\t'||text[p]=='\r'||text[p]=='\n'))++p;
  NormalizedExtended failed{};if(p<text.size()&&text[p]=='-')failed.exponent=0x8000;return {failed,4,p==text.size()?p:0};}
 NormalizedExtended value{};unsigned flags=0;
 if(!scanned->numericDigits.empty()){
  if(scanned->scale>5200){value={{0,0,0,0,0x8000},0x7fff};flags=2;}
  else if(scanned->scale< -5200)flags=1;
  else {
   auto words=decimalExtendedNumeric(scanned->numericDigits);
   value={{uint16_t(words[0]),uint16_t(words[0]>>16),uint16_t(words[1]),uint16_t(words[1]>>16),uint16_t(words[2])},uint16_t(words[2]>>16)};
   value=scaleDecimalExtended(value,int(scanned->scale));
  }
 }
 value.exponent=uint16_t(value.exponent|(scanned->negative?0x8000u:0));
 return {value,flags,scanned->end};
}
