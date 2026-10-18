#pragma once
#include "vmt_keyvalues_candidate.h"
// Deliberately strict flag candidate; duplicates and non-boolean values need
// Source KeyValues evaluation before they can be admitted.
inline bool materialTranslucentCandidate(const VmtNodeCandidate&syntax){auto values=vmtValuesCandidate(syntax,"$translucent");if(values.empty())return false;if(values.size()!=1||(values[0]!="0"&&values[0]!="1"))throw std::runtime_error("unsupported translucent flag");return values[0]=="1";}
// Kept separate from ordinary translucency: native additive uses a distinct
// blend/depth/alpha-write contract and must not be routed as alpha blending.
inline bool materialAdditiveCandidate(const VmtNodeCandidate&syntax){auto values=vmtValuesCandidate(syntax,"$additive");if(values.empty())return false;if(values.size()!=1||(values[0]!="0"&&values[0]!="1"))throw std::runtime_error("unsupported additive flag");return values[0]=="1";}
