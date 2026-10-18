#pragma once
#include <cstdint>
#include <cstddef>
#include <limits>
#include <stdexcept>
template<class Manager,class Services>void studioManagerGrowthCandidate(Manager& manager,int32_t additional,Services& services){
 if(manager.growBy<0)return;
 if(manager.capacity<0||additional<=0)throw std::invalid_argument("invalid vector growth inputs");
 const int64_t requested=int64_t{manager.capacity}+additional;
 int64_t capacity=manager.capacity;
 if(manager.growBy)capacity=((requested-1)/manager.growBy+1)*manager.growBy;
 else{if(!capacity)capacity=8;while(capacity<requested)capacity*=2;}
 if(capacity>(std::numeric_limits<int32_t>::max)())throw std::overflow_error("vector capacity overflow");
 auto* previous=manager.callbacks;
 manager.capacity=static_cast<int32_t>(capacity);
 const auto bytes=static_cast<std::size_t>(capacity)*sizeof(*manager.callbacks);
 manager.callbacks=previous?services.reallocate(previous,bytes):services.allocate(bytes);
}

