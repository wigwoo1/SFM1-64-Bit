#pragma once
#include <atomic>
#include <cstdint>
struct StudioMaterialImmediateLockCandidate{std::atomic<uint32_t> owner{0};uint32_t depth=0;};
// Contended callback must acquire owner and increment depth before returning.
template<class Contended>void studioMaterialImmediateBeginCandidate(StudioMaterialImmediateLockCandidate& lock,uint32_t thread,Contended contended){
 if(lock.owner.load()==thread){++lock.depth;return;}
 uint32_t expected=0;if(lock.owner.compare_exchange_strong(expected,thread)){++lock.depth;return;}contended(lock,thread,0);
}
inline void studioMaterialImmediateEndCandidate(StudioMaterialImmediateLockCandidate& lock){if(--lock.depth==0)lock.owner.exchange(0);}
