#pragma once
#include <cstdint>
inline bool undoAvailableCandidate(std::uint32_t count){return count!=0;}
inline bool redoAvailableCandidate(std::uint32_t count){return count!=0 && count<0x80000000u;}
inline bool undoGroupChangedCandidate(std::uint32_t counter){return counter!=0 && counter<0x80000000u;}
