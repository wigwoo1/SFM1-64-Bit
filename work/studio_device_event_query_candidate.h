#pragma once
#include <cstdint>
template<class Backend>void studioDeviceEventQueryCandidate(int32_t& cached,Backend& backend){
 if(cached!=-1)return;
 auto* query=backend.initialOutput();const auto result=backend.create(8u,query);
 if(static_cast<int32_t>(result)>=0&&query){backend.release(query);cached=1;}else cached=0;
}
