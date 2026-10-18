#pragma once
#include "source_displacement_core_input_candidate.h"
#include "source_displacement_surface_orientation_candidate.h"
#include "source_displacement_grid_positions_candidate.h"
#include "source_displacement_grid_triangles_candidate.h"

struct SourceDisplacementCoreGeometryCandidate {
 uint32_t power=0,contents=0,surfaceFlags=0;
 std::array<std::array<float,3>,4>corners{};
 std::vector<std::array<float,3>>positions;
 std::vector<float>alphas;
 std::vector<SourceDisplacementGridTriangleCandidate>triangles;
};

// Collision-relevant composition of input initialization, quad orientation,
// position generation and triangle creation. Render normals/UVs/LOD node
// interiors are separate; this does not replace those application systems.
inline SourceDisplacementCoreGeometryCandidate sourceDisplacementCoreGeometryCandidate(
 uint32_t power,uint32_t contents,uint32_t encodedFlags,
 const std::array<std::array<float,3>,4>&corners,const std::array<float,3>&start,
 const std::vector<std::array<float,5>>&raw,const std::vector<uint16_t>&tags){
 const auto input=sourceDisplacementCoreInputCandidate(power,encodedFlags,0,0,raw,tags);
 SourceDisplacementSurfaceOrientationCandidate surface;surface.corners=corners;surface.start=start;surface.startCorner=sourceDisplacementStartCornerCandidate(surface);sourceDisplacementRotateSurfaceCandidate(surface);
 std::vector<SourceDisplacementGridOffsetCandidate>offsets;offsets.reserve(raw.size());for(const auto&v:input.vertices)offsets.push_back({{v[0],v[1],v[2]},v[3]});
 SourceDisplacementCoreGeometryCandidate result;result.power=power;result.contents=contents;result.surfaceFlags=input.surfaceFlags;result.corners=surface.corners;result.positions=sourceDisplacementGridPositionsCandidate(power,surface.corners,offsets);for(const auto&v:input.vertices)result.alphas.push_back(v[35]);result.triangles=sourceDisplacementGridTrianglesCandidate(power,input.triangleTags);return result;
}
