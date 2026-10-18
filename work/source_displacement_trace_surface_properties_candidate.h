#pragma once
#include "source_collision_surface_candidate.h"
#include <array>
#include <stdexcept>
#include <utility>
#include <vector>
// Original1014D0A7..1014D0F8 commits property0/1 from the SELECTED runtime
// displacement's +6C/+6E, according to retained trace displacement flag16.
// The provider must resolve these pairs from materials; this class does not
// invent zero properties or derive material identity from final hit position.
class SourceDisplacementTraceSurfacePropertiesCandidate {
    std::vector<std::array<uint16_t,2>>properties_;
public:
    explicit SourceDisplacementTraceSurfacePropertiesCandidate(
        std::vector<std::array<uint16_t,2>>properties):properties_(std::move(properties)){
        if(properties_.size()>65535)throw std::runtime_error("displacement property binding domain exhausted");
    }
    size_t size()const{return properties_.size();}
    uint16_t property(const SourceCollisionSurfaceCandidate&surface,uint16_t displacementFlags)const{
        if(surface.kind!=SourceCollisionSurfaceKindCandidate::Displacement)throw std::runtime_error("displacement property lookup requires displacement surface");
        return properties_.at(surface.index)[(displacementFlags&16u)?1:0];
    }
};
