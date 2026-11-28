#pragma once
#include <cstdint>

// Original10010090: owner is an encoded32-bit handle, not an entity pointer.
// Slots provide a native-width entity identity and the FULL stored serial.
// The caller owns a valid slot table for each nonsentinel low16 index.
template<class SlotLookup>
uintptr_t sourceServerOwnerHandleCandidate(uint32_t handle,SlotLookup&& lookup){
 if(handle==UINT32_MAX)return 0;
 const auto slot=lookup(uint16_t(handle));
 if(slot.serial!=(handle>>16))return 0;
 return slot.entity;
}
