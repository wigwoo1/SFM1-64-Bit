#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <new>
#include <stdexcept>
// Pointer-wide counter portion of CVProfile, not the complete native profile ABI.
// Services own name allocation/release; returned counter addresses borrow this state.
struct StudioProfileCounterCandidate{
 std::array<std::uint32_t,256> values{};
 std::array<std::uint8_t,256> groups{};
 std::array<char*,256> names{};
 std::uint32_t count=0,threadId=0;
};
template<class Ops>std::uint32_t* studioFindProfileCounterCandidate(StudioProfileCounterCandidate& state,const char* name,std::uint32_t group,std::uint32_t& fallback,Ops& ops){
 if(state.count>255)throw std::out_of_range("profile counter count");
 if(state.count==255)return &fallback;
 if(ops.currentThread()!=state.threadId)return &fallback;
 for(unsigned i=0;i<state.count;++i)if(ops.compare(state.names[i],name)==0)return &state.values[i];
 const auto size=std::strlen(name)+1;char* copy=ops.allocate(size);
 // Explicit x64 adaptation: native unchecked allocation would fault while copying.
 if(!copy)throw std::bad_alloc();
 std::memcpy(copy,name,size);const auto index=state.count;
 state.values[index]=0;state.groups[index]=static_cast<std::uint8_t>(group);state.names[index]=copy;++state.count;return &state.values[index];
}
inline void studioResetProfileCounterGroupCandidate(StudioProfileCounterCandidate& state,std::int32_t group){
 if(state.count>255)throw std::out_of_range("profile counter count");
 for(unsigned i=0;i<state.count;++i)if(static_cast<std::int8_t>(state.groups[i])==group)state.values[i]=0;
}
template<class Ops>void studioCleanupProfileCounterNamesCandidate(StudioProfileCounterCandidate& state,Ops& ops){
 if(state.count>255)throw std::out_of_range("profile counter count");
 for(unsigned i=0;i<state.count;++i){ops.release(state.names[i]);state.names[i]=nullptr;}state.count=0;
}
