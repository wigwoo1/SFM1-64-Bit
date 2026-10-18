#pragma once
#include "prepared_session_scene_candidate.h"
#include "fur_scene_upload_candidate.h"
#include "fur_solid_material_candidate.h"
#include "session_camera_settings_candidate.h"
#include "session_camera_view_projection_candidate.h"
#include "film_scene_render_binding_candidate.h"
#include "graph_transform_hierarchy.h"
#include "viewport_camera_guide_geometry_candidate.h"
#include <array>
#include <cmath>
#include <memory>

inline bool sessionCameraFrustumGuidesEnabledCandidate(bool uiEnabled,
    const char* overrideValue, size_t overrideSize) {
    if (!overrideSize) return uiEnabled;
    if (!overrideValue || overrideSize != 1 || (overrideValue[0] != '0' && overrideValue[0] != '1'))
        throw std::runtime_error("camera frustum override must be 0 or 1");
    return overrideValue[0] == '1';
}

inline size_t appendSessionCameraFrustumGuidesCandidate(const Graph& graph,
    const SessionRenderSceneCandidate& scene, PreparedSessionSceneCandidate& prepared,
    bool enabled, std::array<uint8_t, 3> color = {201, 108, 87}) {
    if (!enabled || !scene.width || !scene.height) return 0;
    const auto viewportSettings = readSessionCameraSettingsCandidate(graph, scene.camera);
    auto viewportCameraWorld = graphLocalTransformMatrix(graph, viewportSettings.transform);
    viewportCameraWorld = engineCameraBasisCandidate(viewportCameraWorld);
    const std::array<float, 3> eye{viewportCameraWorld[0][3], viewportCameraWorld[1][3], viewportCameraWorld[2][3]};
    if (viewportSettings.orthographic) return 0;

    FurRibbonGeometryCandidate ribbons;
    size_t guideCount = 0;
    // Session graphs retain other shots' cameras. Only the active shot's DAG
    // can contribute guides, just as it scopes render models and lights.
    const auto binding = readFilmSceneRenderBindingCandidate(graph, scene.shot);
    for (const uint32_t id : binding.cameras) {
        if (id == scene.camera) continue;
        const auto settings = readSessionCameraSettingsCandidate(graph, id);
        if (settings.orthographic) continue; // orthographic guide planes have not been recovered yet
        const auto cameraSettings = readSessionCameraSettingsCandidate(graph, id);
        auto cameraWorld = graphLocalTransformMatrix(graph, cameraSettings.transform);
        cameraWorld = engineCameraBasisCandidate(cameraWorld);
        RenderCameraCandidate projection = sessionCameraViewportProjectionCandidate(graph, id, int(scene.width), int(scene.height), true);
        projection = composeRenderCameraCandidate(projection, cameraRigidInverseCandidate(cameraWorld));
        const auto geometry = viewportCameraFrustumCandidate(projection);
        for (size_t line = 0; line < geometry.lineIndices.size(); line += 2) {
            const auto& from = geometry.corners[geometry.lineIndices[line]];
            const auto& to = geometry.corners[geometry.lineIndices[line + 1]];
            const auto a = scene.viewProjection.clip(from);
            const auto b = scene.viewProjection.clip(to);
            if (a[3] <= 1e-6f || b[3] <= 1e-6f) continue;
            std::array<double, 3> direction{}, towardEye{};
            double distanceSquared = 0;
            for (size_t axis = 0; axis < 3; ++axis) {
                direction[axis] = double(to[axis]) - from[axis];
                const double mid = (double(from[axis]) + to[axis]) * 0.5;
                towardEye[axis] = double(eye[axis]) - mid;
                distanceSquared += towardEye[axis] * towardEye[axis];
            }
            const double lineLength = std::sqrt(direction[0]*direction[0] + direction[1]*direction[1] + direction[2]*direction[2]);
            if (lineLength <= 1e-9) continue;
            for (auto& value : direction) value /= lineLength;
            std::array<double, 3> side{
                direction[1]*towardEye[2] - direction[2]*towardEye[1],
                direction[2]*towardEye[0] - direction[0]*towardEye[2],
                direction[0]*towardEye[1] - direction[1]*towardEye[0]};
            double sideLength = std::sqrt(side[0]*side[0] + side[1]*side[1] + side[2]*side[2]);
            if (sideLength <= 1e-10) {
                side = std::fabs(direction[0]) < 0.9 ? std::array<double,3>{1,0,0} : std::array<double,3>{0,1,0};
                side = {direction[1]*side[2]-direction[2]*side[1],
                        direction[2]*side[0]-direction[0]*side[2],
                        direction[0]*side[1]-direction[1]*side[0]};
                sideLength = std::sqrt(side[0]*side[0] + side[1]*side[1] + side[2]*side[2]);
            }
            for (auto& value : side) value /= sideLength;
            const double distance = std::sqrt(distanceSquared);
            const double halfWidth = std::max(0.01, distance * 2.0 * std::tan(double(viewportCameraHorizontalFovCandidate(viewportSettings.fieldOfView, float(scene.width)/float(scene.height))) * 3.141592653589793 / 360.0) / double(scene.width));
            const uint32_t first = static_cast<uint32_t>(ribbons.vertices.size());
            for (unsigned endpoint = 0; endpoint < 2; ++endpoint) for (unsigned signIndex = 0; signIndex < 2; ++signIndex) {
                const auto& point = endpoint ? to : from;
                const double sign = signIndex ? 1.0 : -1.0;
                SceneGpuVertexCandidate vertex{};
                for (size_t axis = 0; axis < 3; ++axis) {
                    vertex.position[axis] = float(double(point[axis]) + side[axis] * halfWidth * sign);
                    vertex.normal[axis] = float(towardEye[axis] / std::max(distance, 1e-9));
                }
                vertex.uv = {float(signIndex), float(endpoint)};
                if (!sceneGpuVertexFiniteCandidate(vertex)) throw std::runtime_error("camera frustum ribbon overflow");
                ribbons.vertices.push_back(vertex);
            }
            ribbons.indices.insert(ribbons.indices.end(), {first, first+1, first+3, first, first+3, first+2});
            ++guideCount;
        }
    }
    if (!guideCount) return 0;
    auto& upload = prepared.upload;
    auto& inputs = prepared.materialInputs;
    const auto vertices = upload.vertices.size(), indices = upload.indices.size(), draws = upload.opaque.size();
    const auto slots = upload.materials.size(), alpha = prepared.materialAlpha.size();
    const auto entries = inputs.entries.size(), owners = inputs.owners.size(), inputAlpha = inputs.alpha.size();
    try {
        if (slots >= UINT32_MAX) throw std::runtime_error("camera frustum material slot overflow");
        const auto group = inputs.appendOwned(createFurSolidMaterialCandidate(color));
        upload.materials.push_back({group, 0});
        prepared.materialAlpha.push_back(1.0f);
        appendOpaqueFurRibbonsCandidate(upload, ribbons, static_cast<uint32_t>(slots));
    } catch (...) {
        upload.vertices.resize(vertices); upload.indices.resize(indices); upload.opaque.resize(draws);
        upload.materials.resize(slots); prepared.materialAlpha.resize(alpha);
        inputs.entries.resize(entries); inputs.owners.resize(owners); inputs.alpha.resize(inputAlpha);
        throw;
    }
    return guideCount;
}
