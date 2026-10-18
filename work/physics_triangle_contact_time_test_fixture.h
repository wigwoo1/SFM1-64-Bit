#pragma once
#include "source_physics_triangle_contact_time_candidate.h"
struct PhysicsTriangleContactTimeInputFixture {
    SourcePhysicsSimplexCandidate simplex;
    std::array<float,3>direction{};
    float tolerance=0;
};
struct PhysicsTriangleContactTimeCaptureFixture {
    uint32_t query=0;
    PhysicsTriangleContactTimeInputFixture input;
    float output=0;
};
static_assert(sizeof(PhysicsTriangleContactTimeInputFixture)==84&&sizeof(PhysicsTriangleContactTimeCaptureFixture)==92,"contact time fixture extent");
