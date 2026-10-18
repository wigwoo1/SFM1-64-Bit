#pragma once
#include <cstdint>
// Shared branch policy of100BBCF0(channel) and101A4B90(slave).
// Runtime class membership and reference resolution are caller supplied.
inline bool recoveredReverseReferenceAcceptedCandidate(bool attributePresent,bool ownerPresent,bool classAccepted,uint16_t flags,uint32_t attributeName,uint32_t requestedName,bool checkConsistency,bool consistent,uint32_t mode){
 if(!attributePresent||mode>2)return false;
 if(mode>=1&&(flags&0x1000))return false;
 if(mode==2&&!(flags&0x800))return false;
 if(!ownerPresent||!classAccepted)return false;
 if(requestedName!=0xffffffffu&&attributeName!=requestedName)return false;
 return !checkConsistency||consistent;
}
