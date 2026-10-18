#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_render_scene_candidate.h"
#include "session_camera_frustum_guide_candidate.h"
#include "graph_exact_equal.h"
#include "session_render_root_candidate.h"
#include "retained_film_track_candidate.h"
#include <iostream>

static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
static Value floats(std::initializer_list<float> values) {
    Value value;
    value.wire.resize(values.size() * sizeof(float));
    std::memcpy(value.wire.data(), values.begin(), value.wire.size());
    return value;
}
static Attribute attribute(Graph& graph, const char* name, uint8_t type,
                           std::vector<Value> values) {
    return {internString(graph, name), type, std::move(values)};
}
static Value reference(uint32_t id) { Value value; value.reference = id; return value; }
static Graph fixture() {
    Graph graph;
    auto element = [&](const char* type, const char* name) {
        Element value{};
        value.type = internString(graph, type); value.name = internString(graph, name);
        value.id[0] = static_cast<char>(graph.elements.size() + 1);
        graph.elements.push_back(value);
    };
    element("DmeFilmClip", "shot"); // 0
    element("DmeDag", "scene"); // 1
    element("DmeCamera", "view"); // 2
    element("DmeTransform", "view transform"); // 3
    element("DmeCamera", "scene camera"); // 4
    element("DmeTransform", "scene camera transform"); // 5
    element("DmeCamera", "other shot camera"); // 6, intentionally outside scene
    element("DmeTransform", "other shot camera transform"); // 7
    element("DmeFilmClip", "other shot"); // 8
    element("DmeDag", "other scene"); // 9
    graph.elements[0].attributes = {attribute(graph,"camera",1,{reference(2)}),
                                   attribute(graph,"scene",1,{reference(1)})};
    graph.elements[1].attributes = {attribute(graph,"children",15,{reference(2),reference(4)})};
    graph.elements[8].attributes = {attribute(graph,"camera",1,{reference(6)}),
                                   attribute(graph,"scene",1,{reference(9)})};
    graph.elements[9].attributes = {attribute(graph,"children",15,{reference(6)})};
    for (uint32_t id : {2u,4u,6u}) {
        Value flag; flag.wire = {0};
        graph.elements[id].attributes = {attribute(graph,"transform",1,{reference(id+1)}),
            attribute(graph,"fieldOfView",3,{floats({60})}),
            attribute(graph,"znear",3,{floats({1})}),
            attribute(graph,"zfar",3,{floats({20})}), attribute(graph,"ortho",4,{flag})};
        graph.elements[id+1].attributes = {
            attribute(graph,"position",10,{floats({id==2?0.f:5.f,id==6?4.f:0.f,0.f})}),
            attribute(graph,"orientation",13,{floats({0,0,0,1})})};
    }
    return graph;
}
static PreparedSessionSceneCandidate guides(const Graph& graph, bool enabled, size_t& count) {
    auto scene = readSessionRenderSceneCandidate(graph,0,1280,720);
    PreparedSessionSceneCandidate prepared{};
    count = appendSessionCameraFrustumGuidesCandidate(graph,scene,prepared,enabled);
    return prepared;
}
static bool sameMesh(const PreparedSessionSceneCandidate& a, const PreparedSessionSceneCandidate& b) {
    if (a.upload.indices != b.upload.indices || a.upload.vertices.size() != b.upload.vertices.size() ||
        a.upload.materials.size() != b.upload.materials.size()) return false;
    for (size_t i=0;i<a.upload.vertices.size();++i)
        if (std::memcmp(&a.upload.vertices[i],&b.upload.vertices[i],sizeof(SceneGpuVertexCandidate))) return false;
    return true;
}
int main(int argc, char** argv) {
    try {
        static_assert(sizeof(void*)==8,"x64 required");
        auto graph=fixture(); const auto original=graph;
        require(readFilmSceneRenderBindingCandidate(graph,0).cameras==std::vector<uint32_t>{2,4},
                "scene camera traversal escaped its DAG");
        size_t baselineCount=0; const auto baseline=guides(graph,true,baselineCount);
        require(graphExactEqualCandidate(graph,original),"guide generation modified the input graph");
        require(baselineCount>0,"active-scene camera guide absent");
        // A camera belonging only to a different scene cannot affect this shot.
        graph.elements[6].attributes.clear();
        size_t outsideCount=0; const auto outside=guides(graph,true,outsideCount);
        require(outsideCount==baselineCount && sameMesh(baseline,outside),"out-of-scene camera affected guide geometry");
        auto disabled=guides(graph,false,outsideCount);
        require(!outsideCount && disabled.upload.vertices.empty() && disabled.materialInputs.size()==0,
                "disabled guides changed scene data");
        // Shared DAG references must not produce duplicate guides.
        graph.elements[1].attributes[0].values.push_back(reference(4));
        const auto repeated=guides(graph,true,outsideCount);
        require(outsideCount==baselineCount && sameMesh(baseline,repeated),"repeated DAG child duplicated guides");
        // An invalid in-scope camera must still fail instead of being silently skipped.
        graph.elements[4].attributes.clear(); bool rejected=false;
        try { (void)guides(graph,true,outsideCount); } catch(const std::runtime_error&) { rejected=true; }
        require(rejected,"malformed active-scene camera accepted");
        graph=original; graph.elements[1].attributes[0].values={reference(2)};
        const auto activeOnly=guides(graph,true,outsideCount);
        require(!outsideCount && activeOnly.upload.vertices.empty(),"viewing camera drew its own guide");
        graph=original;graph.elements[1].attributes[0].values.push_back(reference(1));rejected=false;
        try { (void)guides(graph,true,outsideCount); } catch(const std::runtime_error&) { rejected=true; }
        require(rejected,"cyclic scene accepted");
        std::cout<<"camera_frustum_scene_scope=passed outside_camera_ignored=1 shared_child_deduplicated=1 "
                   "in_scope_invalid_rejected=1 active_camera_excluded=1 cycle_rejected=1 toggle_preserved=1 guide_edges="<<baselineCount<<'\n';
        if(argc>2)throw std::runtime_error("optional benchmark session only");
        if(argc==2){
            auto benchmark=loadGraph(argv[1]);
            RetainedFilmTrackCandidate track(benchmark,sessionRenderRootCandidate(benchmark));
            auto selected=track.select(50000);require(bool(selected),"benchmark frame 120 has no shot");
            auto binding=readFilmSceneRenderBindingCandidate(benchmark,selected->clip);
            size_t total=0;for(const auto&element:benchmark.elements)
                if(benchmark.strings.at(element.type)=="DmeCamera")++total;
            std::cout<<"benchmark_camera_scope shot="<<selected->clip<<" scene="<<binding.scene
                     <<" session_cameras="<<total<<" scene_cameras="<<binding.cameras.size()<<" ids=";
            for(auto id:binding.cameras)std::cout<<id<<',';
            std::cout<<'\n';
        }
        return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
