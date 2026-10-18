#pragma once
#include "source_physics_line_simplex_candidate.h"
// Test wire retains unused bytes to check that the original and owned routines
// preserve them. They are not interpreted as initialized floating-point data.
struct PhysicsLineSimplexInputFixture {
    SourcePhysicsSimplexCandidate simplex;
    SourcePhysicsSimplexVertexCandidate incoming;
    std::array<float,3>direction{};
};
struct PhysicsLineSimplexOutputFixture {
    SourcePhysicsSimplexCandidate simplex;
    std::array<float,3>direction{};
    uint32_t complete=0;
};
struct PhysicsLineSimplexCaptureFixture {
    uint32_t query=0;
    PhysicsLineSimplexInputFixture input;
    PhysicsLineSimplexOutputFixture output;
};
static_assert(sizeof(SourcePhysicsSimplexVertexCandidate)==16&&sizeof(SourcePhysicsSimplexCandidate)==68,"original scalar simplex layout");
static_assert(sizeof(PhysicsLineSimplexInputFixture)==96&&sizeof(PhysicsLineSimplexOutputFixture)==84&&sizeof(PhysicsLineSimplexCaptureFixture)==184,"simplex test wire extent");
