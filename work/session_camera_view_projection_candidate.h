#pragma once
#include "session_camera_projection_candidate.h"
#include "camera_rigid_inverse_candidate.h"
#include "graph_local_transform.h"
#include "source_camera_sample_candidate.h"
inline RenderCameraCandidate composeRenderCameraCandidate(const RenderCameraCandidate&a,const RenderCameraCandidate&b){RenderCameraCandidate out{};for(size_t r=0;r<4;++r)for(size_t c=0;c<4;++c){float value=a.rows[r*4]*b.rows[c];for(size_t k=1;k<4;++k)value+=a.rows[r*4+k]*b.rows[k*4+c];if(!std::isfinite(value))throw std::runtime_error("camera composition overflow");out.rows[r*4+c]=value;}return out;}
// Native engine option postrotates Z(-90), then X(+90). Exact axis mapping
// below omits the original trigonometric routine's residual rounding.
inline RecoveredMatrix3x4 engineCameraBasisCandidate(const RecoveredMatrix3x4&input){auto out=input;for(size_t r=0;r<3;++r){out[r][0]=-input[r][1];out[r][1]=input[r][2];out[r][2]=-input[r][0];}return out;}
inline RenderCameraCandidate sessionCameraViewProjectionCandidate(const Graph&graph,uint32_t id,int width,int height,bool engineAxes=false){auto settings=readSessionCameraSettingsCandidate(graph,id);auto&transform=graph.elements.at(settings.transform);if(auto scale=namedAttribute(graph,transform,"scale")){if(scale->type!=3||scale->values.size()!=1||wireFloat(scale->values[0])!=1)throw std::runtime_error("scaled camera view pending");}auto local=graphLocalTransformMatrix(graph,settings.transform);if(engineAxes)local=engineCameraBasisCandidate(local);auto view=cameraRigidInverseCandidate(local);return composeRenderCameraCandidate(sessionCameraViewportProjectionCandidate(graph,id,width,height,true),view);}

struct SessionCameraSamplePointsCandidate {
    std::array<float, 2> lens, raster;
};

struct SessionSampledCameraCandidate {
    RenderCameraCandidate viewProjection;
    std::array<float, 3> origin;
};

inline SessionSampledCameraCandidate sessionSampledCameraCandidate(
        const Graph& graph, uint32_t id, int width, int height,
        const SessionCameraSamplePointsCandidate& samples) {
    const auto settings = readSessionCameraSettingsCandidate(graph, id);
    if (width <= 0 || height <= 0 || settings.orthographic)
        throw std::invalid_argument("camera sampling requires a perspective viewport");
    const auto* scale = namedAttribute(graph, graph.elements.at(settings.transform), "scale");
    if (scale && (scale->type != 3 || scale->values.size() != 1 || wireFloat(scale->values[0]) != 1))
        throw std::runtime_error("scaled camera view pending");
    const auto lensValue = [&](const char* name, float fallback) {
        const auto* value = namedAttribute(graph, graph.elements.at(id), name);
        if (!value) return fallback;
        if (value->type != 3 || value->values.size() != 1 || value->values.front().wire.size() != 4)
            throw std::runtime_error("camera optics attribute schema");
        return wireFloat(value->values.front());
    };
    auto local = graphLocalTransformMatrix(graph, settings.transform);
    const std::array<float, 3> origin{local[0][3], local[1][3], local[2][3]};
    const float aspect = float(width) / float(height);
    const SourceCameraLensSampleCandidate lens{
        samples.lens, {local[0][1], local[1][1], local[2][1]}, {local[0][2], local[1][2], local[2][2]},
        lensValue("aperture", .2f), lensValue("focalDistance", 72.f), settings.fieldOfView, aspect
    };
    const SourceCameraRasterSampleCandidate raster{samples.raster, width, height};
    const auto sample = sourceCameraSampleViewCandidate({origin}, lens, raster);
    for (size_t axis = 0; axis < origin.size(); ++axis) local[axis][3] = sample.origin[axis];
    const auto projection = sourceCameraSampleProjectionCandidate(
        viewportCameraHorizontalFovCandidate(settings.fieldOfView, aspect), aspect,
        settings.nearPlane, settings.farPlane, sample);
    return {composeRenderCameraCandidate(projection, cameraRigidInverseCandidate(engineCameraBasisCandidate(local))),
            sample.origin};
}
