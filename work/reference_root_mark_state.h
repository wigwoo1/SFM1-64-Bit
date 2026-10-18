#pragma once
#include "reference_registration_state.h"
// 1035F0D0: returns false when requested low byte exactly matches eligibility.
// A true result means the caller must visit attributes after this state change.
inline bool beginRecoveredReferenceRootMarkCandidate(RecoveredReferenceRegistrationStateCandidate& state,uint32_t enabled,uint32_t context){
 const uint8_t requested=uint8_t(enabled);
 if(uint8_t(state.eligible(context))==requested)return false;
 state.setEligible(requested!=0,context);return true;
}
