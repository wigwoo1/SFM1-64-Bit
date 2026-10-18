#pragma once
#include "flex_weight_resize.h"
// Mirrors native105C2F10, including uint32 wrap for the unset sentinel.
inline void recoveredFlexControllerIndexBindingCandidate(uint32_t& cachedIndex,uint32_t index,std::vector<uint32_t>* modelWeights){cachedIndex=index;if(modelWeights&&index>=modelWeights->size())recoveredFlexWeightResizeCandidate(*modelWeights,index+uint32_t(1));}
