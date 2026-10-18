#pragma once
#include "source_physics_refinement_face_candidate.h"
struct PhysicsRefinementFaceInputFixture {
    uint32_t includeBase=0;
    SourcePhysicsSimplexCandidate simplex;
    std::array<float,3>direction{},initialOutput{};
};
struct PhysicsRefinementFaceOutputFixture {
    SourcePhysicsSimplexCandidate simplex;
    std::array<float,3>normal{};
};
struct PhysicsRefinementFaceCaptureFixture {
    uint32_t query=0;
    PhysicsRefinementFaceInputFixture input;
    PhysicsRefinementFaceOutputFixture output;
};
static_assert(sizeof(PhysicsRefinementFaceInputFixture)==96&&sizeof(PhysicsRefinementFaceOutputFixture)==80&&sizeof(PhysicsRefinementFaceCaptureFixture)==180,"refinement face wire extent");
