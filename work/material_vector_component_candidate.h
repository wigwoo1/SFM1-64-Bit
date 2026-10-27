#pragma once
#include "material_numeric_value_candidate.h"
// Checked immediate vector-only branch of SetVecComponentValue. Unlike
// SetVecValue it preserves the integer cache and unused vector components.
// Native growth stores index, not index+1, in the component-count bits.
inline bool materialVectorComponentCandidate(MaterialNumericValueCandidate&value,uint32_t index,uint32_t bits){
    if((value.typeAndMetadata&15)!=2||index>3)return false;
    if(materialNumericOrderedEqualCandidate(value.vector[index],bits))return true;
    const auto count=(value.typeAndMetadata>>4)&7u;
    if(count>4)return false;
    auto next=value;
    if(count<index){for(uint32_t i=count;i<index;++i)next.vector[i]=0;next.typeAndMetadata=(next.typeAndMetadata&~0x70u)|(index<<4);}
    next.vector[index]=bits;value=next;return true;
}
