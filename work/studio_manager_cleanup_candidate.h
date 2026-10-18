#pragma once
#include <cstdint>
template<class Manager>void studioManagerCleanupCandidate(Manager& manager){
 const int32_t count=manager.count;
 for(int32_t index=0;index<count;++index)manager.callbacks[index]();
}
