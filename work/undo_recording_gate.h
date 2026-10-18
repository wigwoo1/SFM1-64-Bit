#pragma once
#include <cstdint>
// Datamodel 10B37094 slot F4=10353BF0; F8=10353C00.
// File id is native element +38, supplied explicitly, never guessed from DMX.
inline bool recoveredUndoRecordingGate(uint8_t datamodelFlags,bool ownerPresent,uint32_t fileId){
 return (datamodelFlags&8)!=0&&ownerPresent&&fileId!=0xffffffff;
}
