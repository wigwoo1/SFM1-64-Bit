#pragma once
#include <array>
#include <cstdint>
#include <cstring>
// Slot264's localfloat4 upload/cache publication, registers16 and2.
// Ops returns current cache pointers; capacity/lifetime belongs to the owner.
template<class Ops>bool studioShaderConstantPairBlockCandidate(uint32_t index,const std::array<uint32_t,4>& local,Ops& ops){
 if(!std::memcmp(ops.primaryCache()+index*4,local.data(),16))return false;
 ops.submit(index,1,local.data());
 std::memcpy(ops.primaryCache()+index*4,local.data(),16);
 std::memcpy(ops.secondaryCache()+index*4,local.data(),16);
 return true;
}
template<class Ops>void studioShaderConstantPairCandidate(const std::array<uint32_t,4>& parameters,const std::array<uint32_t,4>& color,Ops& ops){
 studioShaderConstantPairBlockCandidate(16,parameters,ops);
 studioShaderConstantPairBlockCandidate(2,color,ops);
}
