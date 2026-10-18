#pragma once
#include <cstdint>
//1035C85B..1035C86B and1035CA43..1035CA53 clear bit10 AFTER callback.
template<class RunPass>void dispatchRecoveredReferenceCleanupCandidate(uint8_t&state,RunPass runPass){if(state&0x10u){runPass();state&=0xefu;}}
