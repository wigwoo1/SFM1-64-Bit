#pragma once
#include <array>
#include <cstdint>
using RecoveredPendingIdentityCandidate=std::array<uint32_t,4>;
//Constructor1035D763 installs103535C0 ->1097BB50, and1039A2D0.
inline uint32_t recoveredPendingIdentityHashCandidate(const RecoveredPendingIdentityCandidate&identity){return identity[0];}
inline bool recoveredPendingIdentityEqualCandidate(const RecoveredPendingIdentityCandidate&a,const RecoveredPendingIdentityCandidate&b){return a==b;}
