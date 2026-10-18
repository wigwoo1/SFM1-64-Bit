#pragma once
#include <cstring>
//100D2090 post-state on original108h byte schema, not a native x64 object ABI.
// Does not claim original store order/concurrent observation equivalence.
inline void initializeModelRenderRecordBytesCandidate(unsigned char* record){
 std::memset(record+0x0c,0,0x4c);
 std::memset(record+0x68,0,0x4c);
}
