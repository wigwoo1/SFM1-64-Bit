#pragma once
#include <cstdint>
inline std::uint64_t controllerStartupSeedCandidate(std::uint64_t filetime,std::uint64_t* output=nullptr){
 auto value=(filetime+std::uint64_t{0xfe624e212ac18000})/std::uint64_t{10000000};
 if(value>std::uint64_t{0x793406fff})value=~std::uint64_t{0};
 if(output)*output=value;return value;
}
