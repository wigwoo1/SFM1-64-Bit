#include "recovered_camera_projection_candidate.h"
#include "viewport_camera_guide_geometry_candidate.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static void near(float actual, float expected) { if (std::abs(actual - expected) >= 1e-4f) { std::fprintf(stderr, "actual=%f expected=%f\n", actual, expected); assert(false); } }

int main() {
    const auto projection = recoveredCameraProjectionCandidate(1.0f, 10.0f, 90.0f, 1.0f, false);
    const auto guide = viewportCameraFrustumCandidate(projection);
    assert(guide.lineIndices.size() == 24);
    near(guide.corners[0][0], -1); near(guide.corners[0][1], -1); near(guide.corners[0][2], -1);
    near(guide.corners[1][0],  1); near(guide.corners[1][1], -1); near(guide.corners[1][2], -1);
    near(guide.corners[2][0], -1); near(guide.corners[2][1],  1); near(guide.corners[2][2], -1);
    near(guide.corners[3][0],  1); near(guide.corners[3][1],  1); near(guide.corners[3][2], -1);
    near(guide.corners[4][0], -10); near(guide.corners[4][1], -10); near(guide.corners[4][2], -10);
    near(guide.corners[7][0],  10); near(guide.corners[7][1],  10); near(guide.corners[7][2], -10);

    auto translated = projection;
    translated.rows[3] -= projection.rows[0] * 3.0f;
    translated.rows[7] += projection.rows[5] * 2.0f;
    translated.rows[11] -= projection.rows[10] * 4.0f;
    translated.rows[15] = 4.0f;
    const auto moved = viewportCameraFrustumCandidate(translated);
    near(moved.corners[0][0], 2); near(moved.corners[0][1], -3); near(moved.corners[0][2], 3);
    near(moved.corners[4][0], -7); near(moved.corners[4][1], -12); near(moved.corners[4][2], -6);

    auto singular = projection;
    singular.rows.fill(0);
    bool rejected = false;
    try { (void)viewportCameraFrustumCandidate(singular); }
    catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
    return 0;
}
