#pragma once
#include "extended_double_round.h"
#include <optional>
struct PackedDouble { uint64_t bits; unsigned status; };
// Verified normal/overflow paths at 0x10A31A05-0x10A31ACC.
// Underflow is deliberately unresolved here: caller must handle nullopt.
inline std::optional<PackedDouble> packExtendedDoubleNormal(NormalizedExtended value) {
 const uint64_t sign=uint64_t(value.exponent&0x8000u)<<48;
 int exponent=int(value.exponent&0x7fffu)-0x3fff;
 auto rounded=roundExtendedTo53(value.significand);
 exponent+=rounded.exponentCarry?1:0;
 if(exponent<=-1023)return std::nullopt;
 if(exponent>=1024)return PackedDouble{sign|0x7ff0000000000000ull,1};
 return PackedDouble{sign|(uint64_t(exponent+1023)<<52)|(rounded.bits&0xfffffffffffffull),0};
}
