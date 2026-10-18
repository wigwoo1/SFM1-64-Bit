#pragma once
#include <array>
#include <cstdint>
template<class Ops>void studioSamplerSubmitCandidate(uint32_t stage,uint32_t type,uint32_t value,Ops& ops){
 if(uint8_t(ops.suppressed()))return;
 if(ops.queueEnabled())ops.queueSampler(std::array<uint32_t,4>{17,stage,type,value});
 else ops.directSampler(stage,type,value);
}
