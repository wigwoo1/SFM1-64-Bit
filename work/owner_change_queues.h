#pragma once
#include <cstdint>
#include <vector>
// Recovered append queues and dirty submission guard. Consumption pending.
struct OwnerChangeQueues {
 std::vector<uint32_t> dirty,attributeMoved;
 void markDirty(uint32_t handle,uint8_t&ownerFlags){if(!(ownerFlags&1))dirty.push_back(handle);ownerFlags|=3;}
 void submitAttributeMoved(uint32_t handle){attributeMoved.push_back(handle);}
};
