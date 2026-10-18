#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "viewport_camera_fov_candidate.h"
#include "session_render_root_candidate.h"
#include "retained_shot_timeline_candidate.h"
#include "session_render_scene_candidate.h"
#include <iomanip>

static void require(bool condition, const char* reason) {
    if (!condition) throw std::runtime_error(reason);
}

int main(int argc, char** argv) {
    try {
        static_assert(sizeof(void*) == 8, "x64 regression required");
        if (argc != 3) throw std::runtime_error("native FOV oracle and disposable benchmark session required");
        std::ifstream oracle(argv[1], std::ios::binary);
        require(bool(oracle), "native FOV oracle missing");
        size_t count = 0, different = 0;
        double maximumError = 0;
        for (;;) {
            std::array<float,3> record{};
            oracle.read(reinterpret_cast<char*>(record.data()), sizeof(record));
            if (oracle.gcount() == 0 && oracle.eof()) break;
            require(oracle.gcount() == sizeof(record), "truncated native FOV record");
            const auto actual = viewportCameraHorizontalFovCandidate(record[0], record[1]);
            const double error = std::abs(double(actual) - record[2]);
            maximumError = std::max(maximumError, error);
            different += std::memcmp(&actual, &record[2], sizeof(actual)) != 0;
            require(error <= 4 * std::numeric_limits<float>::epsilon() * std::max(1.f, record[2]),
                    "x64 viewport FOV differs from installed x86 arithmetic");
            ++count;
        }
        require(count == 42, "native FOV oracle case count");
        size_t rejected = 0;
        for (auto pair : {std::array<float,2>{0,1}, {180,1}, {14,0}, {14,-1},
                          {std::numeric_limits<float>::infinity(),1},
                          {14,std::numeric_limits<float>::quiet_NaN()}}) {
            try { (void)viewportCameraHorizontalFovCandidate(pair[0],pair[1]); }
            catch (const std::runtime_error&) { ++rejected; }
        }
        require(rejected == 6, "invalid FOV/aspect accepted");
        std::cout << "x64_native_viewport_fov_passed cases=" << count
                  << " bitwise_differences=" << different << " max_degrees_error=" << maximumError
                  << " rejected=" << rejected << '\n';

        const double referenceTangent = std::tan(14.0 * 3.141592653589793 / 360.0);
        for (auto size : {std::array<int,2>{800,600}, {1280,720}, {2560,1440}, {3840,2160}, {600,800}}) {
            const float aspect = float(size[0])/float(size[1]);
            const auto fov = viewportCameraHorizontalFovCandidate(14,aspect);
            const auto projection = recoveredCameraProjectionCandidate(3,28377.9f,fov,aspect,true);
            const float distance = 20;
            const float verticalEdge = float(referenceTangent * 0.75 * distance);
            const auto vertical = projection.clip({0,verticalEdge,-distance});
            const auto horizontal = projection.clip({verticalEdge*aspect,0,-distance});
            require(std::abs(vertical[1]/vertical[3]+1) < 2e-6f &&
                    std::abs(horizontal[0]/horizontal[3]-1) < 2e-6f, "reference aspect edge mapping");
            require(std::abs(projection.depth({0,0,-3})) < 2e-6f &&
                    std::abs(projection.depth({0,0,-28377.9f})-1) < 2e-6f, "near/far depth changed");
        }

        auto saved = loadGraph(argv[2]);
        const auto root = sessionRenderRootCandidate(saved);
        RetainedShotTimelineCandidate timeline(std::move(saved),root);
        const auto step = timeline.step(50000,2,2);
        const auto& graph = timeline.graph();
        const auto scene = readSessionRenderSceneCandidate(graph,step.shot.clip,1280,720);
        const auto settings = readSessionCameraSettingsCandidate(graph,scene.camera);
        require(graph.strings.at(graph.elements.at(scene.camera).name) == "camera100" &&
                settings.fieldOfView == 14, "benchmark camera binding changed");
        const auto rawProjection = sessionCameraProjectionCandidate(graph,scene.camera,1280,720,true);
        const auto rawExpected = recoveredCameraProjectionCandidate(settings.nearPlane,settings.farPlane,14,16.f/9.f,true);
        require(rawProjection.rows == rawExpected.rows, "raw CDmeCamera projection contract changed");
        const auto effectiveFov = viewportCameraHorizontalFovCandidate(14,16.f/9.f);
        const auto projection = recoveredCameraProjectionCandidate(settings.nearPlane,settings.farPlane,effectiveFov,16.f/9.f,true);
        const auto local = engineCameraBasisCandidate(graphLocalTransformMatrix(graph,settings.transform));
        const auto expected = composeRenderCameraCandidate(projection,cameraRigidInverseCandidate(local));
        require(scene.viewProjection.rows == expected.rows, "scene camera does not apply native viewport FOV conversion");
        const auto square = sessionCameraViewProjectionCandidate(graph,scene.camera,512,512,true);
        const auto squareExpected = composeRenderCameraCandidate(
            recoveredCameraProjectionCandidate(settings.nearPlane,settings.farPlane,
                viewportCameraHorizontalFovCandidate(14,1),1,true),cameraRigidInverseCandidate(local));
        require(square.rows == squareExpected.rows, "square viewport FOV conversion missing");
        std::cout << std::setprecision(9) << "benchmark_viewport_fov_passed camera=" << scene.camera
                  << " stored_fov=14 effective_fov=" << effectiveFov
                  << " frame=120 tick=50000 models=" << scene.models.size()
                  << " output=1280x720 raw_method_preserved=1\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
