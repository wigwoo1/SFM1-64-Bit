#pragma once
#include <cstdint>
//10975830: formatter supplied by bridge; positive capacity/live storage required.
template<class Format>
std::int32_t labelFormatResultCandidate(char* destination,std::int32_t capacity,
 bool* truncated,Format format){
 const auto result=format(destination,capacity);
 const bool overflow=result<0||result>=capacity;
 if(truncated)*truncated=overflow;
 if(overflow){destination[capacity-1]=0;return capacity;}
 return result;
}
