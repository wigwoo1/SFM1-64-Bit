#pragma once
#include <cstdint>
// Primary10006a90/secondary10006ac0. Descriptorarguments are unused.
// Flag and binding are caller-serialized; clear occurs after successful callback.
template<class Unlock>void studioDynamicMeshBufferUnlockCandidate(uint8_t& locked,uintptr_t buffer,uint32_t count,Unlock unlock){
 if(locked){unlock(buffer,count);locked=0;}
}
