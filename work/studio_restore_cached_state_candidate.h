#pragma once
#include <cstdint>
template<class Services>void studioRestoreCachedStateCandidate(std::uint32_t& cache,
 std::uint32_t key,std::uint32_t value,Services& services){
 if(cache==value)return;
 services.submit(key,value);
 cache=value;
}
