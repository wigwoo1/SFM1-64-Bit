#pragma once
#include <cstdint>
#include <cstring>
// Native10003010: record layout here is the original PE32 24-byte layout,
// not the host Windows API structure. The caller separately truncates to AL.
inline uint32_t studioCpuTopologyCountCandidate(const unsigned char* records,uint32_t count){
 uint32_t result=0;
 for(uint32_t i=0;i<count;++i){uint32_t relationship;std::memcpy(&relationship,records+static_cast<size_t>(i)*24+4,4);if(relationship==0)++result;}
 return result;
}
