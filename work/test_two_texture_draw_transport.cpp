#include "vulkan_prepared_scene_candidate.h"
#include <iostream>
#include <limits>
static std::vector<uint64_t>pipelines;
static std::vector<std::vector<float>>payloads;
static size_t commands=0;
static VKAPI_ATTR void VKAPI_CALL bindPipeline(VkCommandBuffer,VkPipelineBindPoint,VkPipeline p){++commands;pipelines.push_back(uint64_t(p));}
static VKAPI_ATTR void VKAPI_CALL bindDescriptors(VkCommandBuffer,VkPipelineBindPoint,VkPipelineLayout,uint32_t,uint32_t,const VkDescriptorSet*,uint32_t,const uint32_t*){++commands;}
static VKAPI_ATTR void VKAPI_CALL bindVertices(VkCommandBuffer,uint32_t,uint32_t,const VkBuffer*,const VkDeviceSize*){++commands;}
static VKAPI_ATTR void VKAPI_CALL bindIndices(VkCommandBuffer,VkBuffer,VkDeviceSize,VkIndexType){++commands;}
static VKAPI_ATTR void VKAPI_CALL draw(VkCommandBuffer,uint32_t,uint32_t,uint32_t,int32_t,uint32_t){++commands;}
static VKAPI_ATTR void VKAPI_CALL push(VkCommandBuffer,VkPipelineLayout,VkShaderStageFlags stage,uint32_t offset,uint32_t size,const void*data){
 ++commands;if(stage==VK_SHADER_STAGE_VERTEX_BIT){if(offset||size!=64)throw std::runtime_error("camera ABI");return;}
 if(offset!=96||(size!=48&&size!=80))throw std::runtime_error("material ABI");
 const auto*values=static_cast<const float*>(data);payloads.emplace_back(values,values+size/4);
}
int main(){try{
 SceneGpuUploadCandidate scene;scene.vertices.resize(9);scene.indices={0,1,2,3,4,5,6,7,8};
 scene.materials={{0,0},{0,1},{0,2}};
 scene.opaque={{0,0,0,3,0,0},{0,1,3,3,1,1},{0,2,6,3,2,2}};
 scene.translucent={{0,0,0,3,0,0}};
 VulkanPreparedSceneResourcesCandidate resources{};resources.vertices=VkBuffer(1);resources.indices=VkBuffer(2);
 resources.vertexCapacity=scene.vertices.size()*sizeof(SceneGpuVertexCandidate);resources.indexCapacity=scene.indices.size()*4;
 resources.opaque=VkPipeline(10);resources.translucent=VkPipeline(20);resources.layout=VkPipelineLayout(1);
 resources.materials={VkDescriptorSet(1),VkDescriptorSet(2),VkDescriptorSet(3)};
 resources.baseTextureTransform.assign(3,identityMaterialUvTransformCandidate());resources.baseTextureTransform[1][2]=.25f;
 resources.secondTextureTransform.assign(3,identityMaterialUvTransformCandidate());resources.secondTextureTransform[1][6]=.75f;
 resources.materialPipelines={VK_NULL_HANDLE,VkPipeline(30),VK_NULL_HANDLE};resources.twoTextureSlots={0,1,0};
 std::vector<float>alpha{1,1,1};RenderCameraCandidate camera{};
 VulkanSceneCommandFunctionsCandidate functions{bindPipeline,bindDescriptors,bindVertices,bindIndices,draw};
 auto record=[&](const auto&input){recordVulkanPreparedSceneCandidate(functions,push,VkCommandBuffer(1),scene,camera,alpha,input);};
 record(resources);
 if(pipelines!=std::vector<uint64_t>{10,30,10,20}||payloads.size()!=4||payloads[0].size()!=12||payloads[1].size()!=20||payloads[2].size()!=12||payloads[3].size()!=12)throw std::runtime_error("pipeline restoration or payload shape");
 if(payloads[1][0]!=1||payloads[1][3]!=1||payloads[1][6]!=.25f||payloads[1][18]!=.75f)throw std::runtime_error("independent transform payload identity");
 size_t rejected=0;
 auto reject=[&](const auto&invalid){commands=0;try{record(invalid);}catch(const std::runtime_error&){++rejected;if(commands)throw std::runtime_error("invalid resources emitted partial commands");return;}throw std::runtime_error("invalid resources accepted");};
 auto invalid=resources;invalid.materialPipelines.pop_back();reject(invalid);
 invalid=resources;invalid.secondTextureTransform.pop_back();reject(invalid);
 invalid=resources;invalid.secondTextureTransform[1][2]=std::numeric_limits<float>::quiet_NaN();reject(invalid);
 invalid=resources;invalid.twoTextureSlots[1]=2;reject(invalid);
 invalid=resources;invalid.materialPipelines[1]=VK_NULL_HANDLE;reject(invalid);
 invalid=resources;invalid.secondTextureTransform.clear();reject(invalid);
 invalid=resources;invalid.alphaCutoff={0,.5f,0};reject(invalid);
 alpha[1]=.5f;reject(resources);alpha[1]=1;
 commands=0;pipelines.clear();payloads.clear();resources.twoTextureSlots.clear();resources.secondTextureTransform.clear();resources.materialPipelines.clear();
 record(resources);if(pipelines!=std::vector<uint64_t>{10,20})throw std::runtime_error("legacy pipeline behavior changed");
 for(const auto&payload:payloads)if(payload.size()!=12)throw std::runtime_error("legacy push ABI changed");
 std::cout<<"PASS: pipeline restoration across materials/phases, independent 80-byte two-texture payload, legacy ABI, eight invalid inputs emit zero commands; instrumented CPU transport only\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
