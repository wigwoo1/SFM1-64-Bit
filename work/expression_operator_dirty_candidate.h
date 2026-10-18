#pragma once
#include <cstdint>
// Recovered expression operator object byte at this+0x42. The owned runtime
// supplies that byte independently of serialized attributes; dirty propagation
// into it still needs native recovery.
inline bool expressionOperatorDirtyCandidate(uint8_t flags){return (flags&0x02)!=0;}
inline void clearExpressionOperatorDirtyCandidate(uint8_t&flags){flags&=0xfd;}
