#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "graph_transform_hierarchy.h"
#include "dag_world_fixture.h"
#include "session_ifm_lighting_origin_candidate.h"
#include <functional>
struct LightingSafetyBounds {
    unsigned queries=0;
    void synchronize(){++queries;}
    std::optional<IfmBoundsCandidate>animatedBounds(){return IfmBoundsCandidate{{{-4,-6,-8},{8,10,12}}};}
    const IfmStudioBoundsCandidate*studio(){throw std::runtime_error("unexpected studio query");}
    IfmBoundsCandidate modelBounds(){throw std::runtime_error("unexpected model bounds");}
    std::optional<RecoveredMatrix3x4>childTransform(){return {};}
    std::optional<IfmFirstBonePoseCandidate>firstBonePose(){return {};}
    RecoveredMatrix3x4 wrapperTransform(){throw std::runtime_error("unexpected wrapper query");}
};
int main(){try{
    unsigned checks=0;auto require=[](bool ok){if(!ok)throw std::runtime_error("lighting composition safety assertion");};
    std::vector<DagWorldFixtureNode>nodes(2);nodes[0].position={10,20,30};nodes[1].parent=0;nodes[1].position={3,4,5};
    auto graph=dagWorldFixtureGraph(nodes);const uint32_t modelType=uint32_t(graph.strings.size());graph.strings.push_back("DmeGameModel");graph.strings.push_back("illumPositionDag");graph.elements[0].type=modelType;
    IfmRenderableBoundsStateCandidate state{true,true,true,true,0,3,0,0,1};LightingSafetyBounds bounds;GraphTransformHierarchy hierarchy(graph);
    // Erase test callback types so intentionally throwing callbacks do not
    // produce spurious unreachable-code warnings in the templated adapter.
    unsigned headers=0,worlds=0;auto run=[&](const std::function<std::optional<int32_t>()>&index){return sessionIfmLightingOriginCandidate(graph,0,state,[&](uint32_t id){return id<2;},[&]()->std::optional<int32_t>{++headers;return index();},[&](uint32_t id){++worlds;return hierarchy.world(id);},bounds);};
    const auto midpoint=run([]{return 0;});require(midpoint.point==std::optional<std::array<float,3>>({2,2,2})&&midpoint.boundsQueried&&midpoint.headerQueried&&!midpoint.selectedDag&&headers==1&&worlds==0&&bounds.queries==1);++checks;
    const auto positive=run([]{return 1;});require(!positive.point&&positive.headerQueried&&!positive.boundsQueried&&bounds.queries==1);++checks;
    Value ref;ref.reference=1;graph.elements[0].attributes.push_back(Attribute{modelType+1,1,{ref}});
    const auto explicitDag=run([]()->int32_t{throw std::runtime_error("present attribute queried header");});require(explicitDag.point==std::optional<std::array<float,3>>({13,24,35})&&explicitDag.selectedDag==1u&&!explicitDag.headerQueried&&!explicitDag.boundsQueried&&worlds==1);++checks;
    graph.elements[0].attributes.back().values[0].reference=UINT32_MAX;const auto child=run([]()->int32_t{throw std::runtime_error("child fallback queried header");});require(child.selectedDag==1u&&child.point==explicitDag.point&&worlds==2);++checks;
    for(auto&a:graph.elements[0].attributes)if(graph.strings[a.name]=="children"){Value null;null.reference=UINT32_MAX;a.values.insert(a.values.begin(),null);}
    const auto noPoint=run([]()->int32_t{throw std::runtime_error("null child queried header");});require(!noPoint.point&&!noPoint.selectedDag&&!noPoint.headerQueried&&!noPoint.boundsQueried&&worlds==2);++checks;
    // Malformed schema and nonfinite point must fail before a result is returned.
    auto rejects=[&](auto fn){bool caught=false;try{fn();}catch(const std::runtime_error&){caught=true;}require(caught);++checks;};
    graph.elements[0].attributes.back().type=2;rejects([&]{run([]{return 0;});});graph.elements[0].attributes.back().type=1;
    graph.elements[0].attributes.back().values[0].reference=1;
    rejects([&]{sessionIfmLightingOriginCandidate(graph,0,state,[](uint32_t){return true;},[]()->std::optional<int32_t>{return 0;},[](uint32_t){RecoveredMatrix3x4 m{};m[0][3]=std::numeric_limits<float>::infinity();return m;},bounds);});
    graph.elements[0].attributes.pop_back();rejects([&]{run([]()->std::optional<int32_t>{return {};});});
    require(bounds.queries==1);++checks;
    std::cout<<"PASS lighting composition safety checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
