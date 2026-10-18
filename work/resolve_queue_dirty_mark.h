#pragma once
#include <cstdint>
// Native1035DB90 queues once while bit1 clear, then ORs element byte flags3.
template<class Enqueue>inline void recoveredResolveQueueDirtyMarkCandidate(uint8_t&flags,uint32_t handle,Enqueue enqueue){if(!(flags&1))enqueue(handle);flags|=3;}
