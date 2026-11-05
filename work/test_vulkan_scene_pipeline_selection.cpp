// Run the existing bridge/recorder tests, then exercise the new per-draw route
// through that same prepared bridge. Instrumentation does not execute a GPU.
#define main precedingSceneCommandsMain
#include "test_vulkan_scene_commands.cpp"
#undef main

static void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}
template<class F>static void refuses(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}require(failed,"invalid per-draw selection accepted");require(events.empty(),"refused selection emitted partial Vulkan commands");}
int main(){try{
 require(precedingSceneCommandsMain()==0,"existing scene bridge regressions");events.clear();
 VulkanSceneCommandFunctionsCandidate fn{pipeline,descriptors,vertices,indices,draw};Prepared scene{};scene.upload.vertices.resize(3);scene.upload.indices={0,1,2,0,1,2,0,1,2,0,1,2};scene.upload.materials={{0,0}};
 for(uint32_t n=0;n<4;++n)scene.upload.opaque.push_back({0,0,n*3,3,0,0});scene.camera.rows={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};scene.materialAlpha={1};scene.materialInputs.models.resize(1);scene.materialInputs.models[0].materials.resize(1);
 RetainedSceneMaterialGpuCacheCandidate<Images>cache([](const auto&input){auto images=std::make_unique<Images>();for(size_t n=0;n<input.materials.size();++n)images->entries.push_back({VkDescriptorSet(30+n)});return images;});PreparedSceneGpuBridgeCandidate<Images>bridge(cache);
 auto resources=bridge.resources(scene,VkBuffer(1),VkBuffer(2),3*sizeof(SceneGpuVertexCandidate),12*4,VkPipeline(10),VkPipeline(20),VkPipelineLayout(1));resources.materialPipelines={VkPipeline(77)};
 std::vector<uint32_t>selected;resources.drawPipeline=[&](const auto&d,VkPipeline fallback){require(fallback==VkPipeline(77),"material pipeline fallback identity");selected.push_back(d.firstIndex);return d.firstIndex==3?fallback:VkPipeline(90);};
 resources.beforeDraw=[](const auto&d){events.push_back(200+d.firstIndex);};
 bridge.record(scene,fn,push,reinterpret_cast<VkCommandBuffer>(1),resources);
 require(selected==std::vector<uint32_t>{0,3,6,9},"selector order/count");
 require(events==std::vector<uint64_t>{1,2,10,90,30,50,1100,200,100,77,30,1100,203,103,90,30,1100,206,106,30,1100,209,109},"same-material HDR/legacy/HDR/repeated pipeline cache or hook order");
 events.clear();size_t rejected=0;
 for(size_t fail=0;fail<4;++fail){size_t at=0;resources.drawPipeline=[&](const auto&,VkPipeline fallback){return at++==fail?VK_NULL_HANDLE:fallback;};refuses([&]{bridge.record(scene,fn,push,reinterpret_cast<VkCommandBuffer>(1),resources);});require(at==fail+1,"invalid selector evaluated twice");++rejected;}
 for(bool allocation:{false,true}){size_t at=0;resources.drawPipeline=[&](const auto&,VkPipeline fallback){if(at++==3){if(allocation)throw std::bad_alloc();throw std::runtime_error("owned selector failure");}return fallback;};refuses([&]{bridge.record(scene,fn,push,reinterpret_cast<VkCommandBuffer>(1),resources);});require(at==4,"throwing selector count");++rejected;}
 size_t calls=0;resources.drawPipeline=[&](const auto&,VkPipeline fallback){++calls;return fallback;};auto bad=scene;bad.upload.indices.back()=99;refuses([&]{bridge.record(bad,fn,push,reinterpret_cast<VkCommandBuffer>(1),resources);});require(calls==0,"invalid input reached selector");++rejected;
 // The pass fallback is evaluated separately for opaque and translucent draws.
 resources.materialPipelines.clear();std::vector<uint64_t>fallbacks;resources.drawPipeline=[&](const auto&d,VkPipeline fallback){fallbacks.push_back(uint64_t(fallback));return d.firstIndex%6==0?VkPipeline(90):fallback;};auto passes=scene;passes.upload.opaque.resize(2);passes.upload.translucent.assign(scene.upload.opaque.begin()+2,scene.upload.opaque.end());bridge.record(passes,fn,push,reinterpret_cast<VkCommandBuffer>(1),resources);require(fallbacks==std::vector<uint64_t>{10,10,20,20},"pass fallback identity");std::vector<uint64_t>bound;for(auto e:events)if(e==10||e==20||e==90)bound.push_back(e);require(bound==std::vector<uint64_t>{10,90,10,20,90,20},"opaque/translucent pipeline restoration");events.clear();
 // Omitting the selector still follows the old material cache behavior.
 resources.drawPipeline={};resources.beforeDraw={};resources.materialPipelines={VkPipeline(77)};bridge.record(scene,fn,push,reinterpret_cast<VkCommandBuffer>(1),resources);require(events==std::vector<uint64_t>{1,2,10,77,30,50,1100,100,30,1100,103,30,1100,106,30,1100,109},"omitted selector changed recording");
 std::cout<<"PASS prepared_per_draw_pipeline=1 same_material_hdr_legacy_hdr=1 repeated_pipeline_not_rebound=1 material_and_pass_fallbacks=1 selector_once_in_preflight=1 invalid_selector_zero_commands="<<rejected<<" post_material_hook=1 legacy_sequence_unchanged=1 GPU_execution=UNVERIFIED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
