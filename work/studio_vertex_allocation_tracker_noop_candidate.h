#pragma once
#include <cstdint>
// Verified shaderapidx9 local VBAllocTracker001 provider only.
// Startup may select a different provider; this does not own GPU resources.
struct StudioVertexAllocationTrackerNoOpCandidate{
 void registerBuffer(std::uintptr_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uintptr_t,std::uintptr_t) noexcept {}
 void removeBuffer(std::uintptr_t) noexcept {}
 void setContext(const char*) noexcept {}
};
