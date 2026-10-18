#pragma once
#include "extended_normalize.h"
// 0x10A316DF-0x10A317CD: guard set AND lower discarded bits nonzero.
// Exact half cases truncate magnitude, independent of retained parity.
struct DoubleSignificand{uint64_t bits;bool exponentCarry;};
inline DoubleSignificand roundExtendedTo53(const ExtendedLimbs&limbs){
 uint64_t upper=uint64_t(limbs[1])|(uint64_t(limbs[2])<<16)|(uint64_t(limbs[3])<<32)|(uint64_t(limbs[4])<<48);
 uint64_t retained=upper>>11;
 uint32_t discarded=uint32_t(limbs[0])|(uint32_t(limbs[1]&0x7ffu)<<16);
 if(discarded>0x4000000u)++retained;
 bool carry=(retained>>53)!=0;
 if(carry)retained=uint64_t(1)<<52;
 return {retained,carry};
}
