#pragma once
#include "decimal_scanned_value.h"
#include "extended_double.h"
struct DecimalConversion {uint64_t bits;unsigned flags;size_t consumed;};
// Recovered conversion structure wrapper 0x10A1DD78. Outer CRT end-pointer
// and errno handling are separate, not silently inferred from these flags.
inline DecimalConversion nativeDecimalConversion(std::string_view text){
 auto parsed=scannedDecimalExtended(text);
 if(parsed.parserFlags&4)return {0,0x200,parsed.end};
 auto packed=packExtendedDouble(parsed.value);unsigned flags=0;
 if((parsed.parserFlags&2)||packed.status==1)flags=0x80;
 if((parsed.parserFlags&1)||packed.status==2)flags|=0x100;
 return {packed.bits,flags,parsed.end};
}
