#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "vulkan_scene_commands_candidate.h"
#include "retained_obj_scene_candidate.h"
#include <cstdio>
#include <cstdlib>
static std::vector<std::pair<uint32_t,uint32_t>> draws;static std::vector<VkDescriptorSet> descriptorsUsed;
static VKAPI_ATTR void VKAPI_CALL pipeline(VkCommandBuffer,VkPipelineBindPoint,VkPipeline){}
static VKAPI_ATTR void VKAPI_CALL descriptor(VkCommandBuffer,VkPipelineBindPoint,VkPipelineLayout,uint32_t,uint32_t,const VkDescriptorSet* set,uint32_t,const uint32_t*){descriptorsUsed.push_back(*set);}
static VKAPI_ATTR void VKAPI_CALL vertex(VkCommandBuffer,uint32_t,uint32_t,const VkBuffer*,const VkDeviceSize*){}
static VKAPI_ATTR void VKAPI_CALL index(VkCommandBuffer,VkBuffer,VkDeviceSize,VkIndexType){}
static VKAPI_ATTR void VKAPI_CALL draw(VkCommandBuffer,uint32_t count,uint32_t instances,uint32_t first,int32_t offset,uint32_t){if(instances!=1||offset)std::abort();draws.push_back({count,first});}
static void check(bool b){if(!b)std::abort();}
struct Prepared {SceneGpuUploadCandidate upload;SceneMaterialInputsCandidate materialInputs;std::vector<float> materialAlpha;};
int main(){std::istringstream source("v 0 0 0\nv 1 0 0\nv 0 1 0\nv 1 1 0\nusemtl A\nf 1 2 3\nusemtl B\nf 2 4 3\n");RetainedObjSceneCandidate imported(studioReadObjCandidate(source));Prepared prepared;prepared.upload.vertices.resize(3);prepared.upload.indices={0,1,2};prepared.upload.materials={{0,0}};prepared.upload.opaque={{0,0,0,3,0,0}};prepared.materialAlpha={1};auto original=std::make_shared<ModelRenderInputsCandidate>();original->materials.resize(1);prepared.materialInputs.appendOwned(original);
 imported.append(prepared);check(prepared.upload.vertices.size()==9&&prepared.upload.indices==std::vector<uint32_t>({0,1,2,3,4,5,6,7,8}));check(prepared.upload.materials.size()==3&&prepared.materialInputs.size()==2&&prepared.materialInputs.ownerAt(1));check(prepared.materialAlpha==std::vector<float>({1,1,1}));for(const auto& m:prepared.materialInputs.at(1).materials)check(m.image.rgba==std::vector<uint8_t>({255,255,255,255}));
 VulkanSceneCommandFunctionsCandidate fn{pipeline,descriptor,vertex,index,draw};recordVulkanSceneCommandsCandidate(fn,reinterpret_cast<VkCommandBuffer>(1),prepared.upload,VkBuffer(1),VkBuffer(2),VkPipeline(3),VkPipeline(4),VkPipelineLayout(5),{VkDescriptorSet(6),VkDescriptorSet(7),VkDescriptorSet(8)});
 check(draws==std::vector<std::pair<uint32_t,uint32_t>>({{3,0},{3,3},{3,6}})&&descriptorsUsed==std::vector<VkDescriptorSet>({VkDescriptorSet(6),VkDescriptorSet(7),VkDescriptorSet(8)}));auto owner=prepared.materialInputs.ownerAt(1);Prepared next;imported.append(next);check(next.materialInputs.ownerAt(0)==owner&&next.upload.indices.front()==0);
 std::puts("obj_retained_scene_bridge_source_offsets_material_identity_and_vulkan_commands=passed backend=instrumented GPU_execution=unverified");
}

