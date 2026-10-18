#pragma once
#include <vector>
#include <cstdint>
#include <stdexcept>
// Source SDK GetBodygroup arithmetic; installed SFM parity remains unverified.
inline std::vector<uint32_t> modelBodySelectionCandidate(uint32_t body,const std::vector<uint32_t>&counts,const std::vector<uint32_t>&bases){
 if(counts.size()!=bases.size())throw std::runtime_error("body metadata count mismatch");std::vector<uint32_t> result;result.reserve(counts.size());
 for(size_t n=0;n<counts.size();++n){if(!counts[n]||!bases[n])throw std::runtime_error("invalid body metadata");result.push_back(counts[n]<=1?0:(body/bases[n])%counts[n]);}return result;
}
