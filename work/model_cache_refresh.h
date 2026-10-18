#pragma once
#include <cstdint>
#include <optional>
#include <string>
struct RecoveredModelCacheState {uint16_t handle=0xffff;bool dirty=true;};
// Native105C2640 acquires before releasing, even for identical handles.
template<class Acquire,class Release>
inline void recoveredModelCacheRefreshCandidate(RecoveredModelCacheState&state,const std::optional<std::string>&name,Acquire acquire,Release release){uint16_t next=0xffff;if(name&&!name->empty())next=static_cast<uint16_t>(acquire(*name));if(state.handle!=0xffff)release(state.handle);state.handle=next;state.dirty=false;}
