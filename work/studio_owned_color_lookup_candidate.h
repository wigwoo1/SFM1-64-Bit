#pragma once
#include "studio_owned_math_startup_candidate.h"
#include "studio_color_lookup_candidate.h"
struct StudioOwnedColorLookupCandidate {
 const StudioMathTablesOwnerCandidate& math;
 StudioColorLookupRoundingCandidate rounding=StudioColorLookupRoundingCandidate::NearestEven;
 float lookup(float input) const {
  if(!math.startup.initialized)throw std::logic_error("color lookup requires math initialization");
  return studioColorLookupCandidate(input,math.gamma.fixedPower22,rounding);
 }
};
