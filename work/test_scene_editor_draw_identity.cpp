#define main previousSceneCommandTests
#include "test_vulkan_scene_commands.cpp"
#undef main
#include "scene_editor_draw_identity_candidate.h"
#include <tuple>

static auto identity(const SceneGpuDrawCandidate& d){
    return std::make_tuple(d.model,d.range,d.firstIndex,d.indexCount,d.materialIndex,d.materialSlot);
}
static void need(bool value,const char*message){if(!value)throw std::runtime_error(message);}
struct FlatRange {uint32_t firstIndex,indexCount,materialIndex;};
int main(){try{
    static_assert(sizeof(void*)==8,"native x64 required");
    need(previousSceneCommandTests()==0,"existing prepared bridge/recorder tests");events.clear();
    SceneGpuUploadCandidate source;source.vertices.resize(3);source.indices={0,1,2,0,1,2,0,1,2,0,1,2,0,1,2};
    source.materials={{2,1},{0,0}};
    source.opaque={{2,9,0,3,1,0},{0,5,3,3,0,1},{2,3,6,3,1,0}};
    source.translucent={{0,7,9,3,0,1},{2,9,12,3,1,0}};
    const auto saved=source;
    std::vector<FlatRange> flat{{0,3,0},{3,3,1},{6,3,0},{9,3,1},{12,3,0}};
    auto draws=sceneEditorDrawIdentitiesCandidate(source,flat);
    const std::vector<SceneGpuDrawCandidate> expected{{2,9,0,3,1,0},{0,5,3,3,0,1},{2,3,6,3,1,0},{0,7,9,3,0,1},{2,9,12,3,1,0}};
    need(draws.size()==expected.size(),"identity count");size_t previousWrong=0;
    for(size_t n=0;n<draws.size();++n){need(identity(draws[n])==identity(expected[n]),"lost retained draw field");previousWrong+=draws[n].range!=n;}
    need(previousWrong==5,"fixture did not reproduce previous flattening error");
    // Same visible opaque order and reversed translucent depth order as selected
    // by the editor. Material slots and offsets stay intact; local ranges survive.
    auto view=source;view.opaque={draws[0],draws[1],draws[2]};view.translucent={draws[4],draws[3]};
    Prepared prepared{view,{}, {1,.5f},{}};prepared.camera.rows={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    prepared.materialInputs.models.resize(3);prepared.materialInputs.models[0].materials.resize(1);prepared.materialInputs.models[2].materials.resize(2);
    RetainedSceneMaterialGpuCacheCandidate<Images> cache([](const auto&input){auto p=std::make_unique<Images>();for(size_t n=0;n<input.materials.size();++n)p->entries.push_back({VkDescriptorSet(30+n)});return p;});
    PreparedSceneGpuBridgeCandidate<Images> bridge(cache);
    auto resources=bridge.resources(prepared,VkBuffer(1),VkBuffer(2),3*sizeof(SceneGpuVertexCandidate),15*4,VkPipeline(10),VkPipeline(20),VkPipelineLayout(1));
    std::vector<decltype(identity(draws[0]))> selected,recorded;
    resources.drawPipeline=[&](const auto&d,VkPipeline fallback){selected.push_back(identity(d));return d.model==2&&d.range==9?VkPipeline(90):fallback;};
    resources.beforeDraw=[&](const auto&d){recorded.push_back(identity(d));};
    VulkanSceneCommandFunctionsCandidate fn{pipeline,descriptors,vertices,indices,draw};
    bridge.record(prepared,fn,push,reinterpret_cast<VkCommandBuffer>(1),resources);
    const std::vector<decltype(identity(draws[0]))> ordered{identity(expected[0]),identity(expected[1]),identity(expected[2]),identity(expected[4]),identity(expected[3])};
    need(selected==ordered&&recorded==ordered,"bridge callbacks lost model-local range or depth order");
    std::vector<uint64_t> offsets;for(auto e:events)if(e>=100&&e<200)offsets.push_back(e);
    need(offsets==std::vector<uint64_t>{100,103,106,112,109},"indexed draw sequence changed");events.clear();
    size_t refused=0;
    auto reject=[&](auto s,auto f){bool failed=false;try{(void)sceneEditorDrawIdentitiesCandidate(s,f);}catch(const std::runtime_error&){failed=true;}need(failed,"invalid editor/source association accepted");need(events.empty(),"identity refusal emitted commands");++refused;};
    auto badFlat=flat;badFlat.pop_back();reject(source,badFlat);
    badFlat=flat;badFlat[0].firstIndex=3;reject(source,badFlat);
    badFlat=flat;badFlat[2].indexCount=6;reject(source,badFlat);
    badFlat=flat;badFlat[3].materialIndex=0;reject(source,badFlat);
    auto bad=source;bad.opaque[0].materialSlot=99;badFlat=flat;badFlat[0].materialIndex=99;reject(bad,badFlat);
    bad=source;bad.opaque[0].model=1;reject(bad,flat);
    bad=source;bad.opaque[0].materialIndex=0;reject(bad,flat);
    bad=source;bad.opaque[0].firstIndex=UINT32_MAX;badFlat=flat;badFlat[0].firstIndex=UINT32_MAX;reject(bad,badFlat);
    bad=source;bad.opaque[0].indexCount=2;badFlat=flat;badFlat[0].indexCount=2;reject(bad,badFlat);
    need(sceneEditorDrawIdentitiesCandidate(SceneGpuUploadCandidate{},std::vector<FlatRange>{}).empty(),"blank draw packet");
    // Preserve the old recorder's zero-count range behavior, without inventing
    // a new validation policy as part of this identity-only correction.
    bad=source;bad.opaque[0].indexCount=0;badFlat=flat;badFlat[0].indexCount=0;
    need(sceneEditorDrawIdentitiesCandidate(bad,badFlat)[0].indexCount==0,"zero-count compatibility changed");
    source={};for(size_t n=0;n<draws.size();++n)need(identity(draws[n])==identity(expected[n]),"frame replacement invalidated copied identities");
    need(saved.indices==view.indices,"index source changed");
    std::cout<<"PASS editor_retained_draw_identity=1 previous_wrong_ranges="<<previousWrong<<" bridge_callbacks=5 preserved_depth_order=1 invalid_associations="<<refused<<" blank_and_zero=1 copied_values=1 GPU_execution=UNVERIFIED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
