#pragma once
#include <cstdint>
// Native kind-2 retain/release alters only packed bits20..29, modulo1024.
// Collection scheduling and generation lookup are separate native operations.
inline uint32_t retainUndoReferenceCount(uint32_t packed){return (packed&~0x3ff00000u)|((((packed>>20)+1u)&0x3ffu)<<20);}
inline uint32_t releaseUndoReferenceCount(uint32_t packed){return (packed&~0x3ff00000u)|((((packed>>20)-1u)&0x3ffu)<<20);}
