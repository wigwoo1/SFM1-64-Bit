#include "prepared_scene_gpu_bridge_candidate.h"
#include <iostream>
#include <limits>
struct Images{struct Entry{VkDescriptorSet set;};std::vector<Entry>entries;void upload(VkCommandBuffer){}};
struct Inputs{std::vector<ModelRenderInputsCandidate>models;const auto&at(size_t n)const{return models.at(n);}};
struct Prepared{SceneGpuUploadCandidate upload;RenderCameraCandidate camera;std::vector<float>materialAlpha;Inputs materialInputs;};
int main(){try{
 Prepared scene;scene.upload.vertices.resize(3);scene.upload.indices={0,1,2};scene.upload.materials={{0,0},{0,1}};scene.materialAlpha={1,1};scene.materialInputs.models.resize(1);scene.materialInputs.models[0].materials.resize(2);
 auto&material=scene.materialInputs.models[0].materials[1];material.unlitTwoTexture=true;material.additive=true;material.image={1,1,{255,255,255,255}};material.secondBaseImage={1,1,{255,255,255,255}};material.secondTextureTransform[2]=.75f;
 material.source.syntax=parseVmtCandidate("UnlitTwoTexture {$color \"[1 1 1]\" $cloakfactor 0}");
 RetainedSceneMaterialGpuCacheCandidate<Images>cache([](const auto&input){auto result=std::make_unique<Images>();for(size_t n=0;n<input.materials.size();++n)result->entries.push_back({VkDescriptorSet(100+n)});return result;});
 PreparedSceneGpuBridgeCandidate<Images>bridge(cache);
 auto generate=[&](VkPipeline selected){return bridge.resources(scene,VkBuffer(1),VkBuffer(2),3*sizeof(SceneGpuVertexCandidate),12,VkPipeline(10),VkPipeline(20),VkPipelineLayout(1),std::nullopt,selected);};
 auto result=generate(VkPipeline(30));
 if(result.materialPipelines!=std::vector<VkPipeline>{VK_NULL_HANDLE,VkPipeline(30)}||result.twoTextureSlots!=std::vector<uint8_t>{0,1}||result.secondTextureTransform[1][2]!=.75f)throw std::runtime_error("bridge slot/payload identity");
 auto legacy=generate(VK_NULL_HANDLE);if(!legacy.materialPipelines.empty()||!legacy.twoTextureSlots.empty())throw std::runtime_error("legacy bridge changed");
 const auto saved=material;size_t rejected=0;
 auto reject=[&]{size_t allocations=0;RetainedSceneMaterialGpuCacheCandidate<Images>freshCache([&](const auto&){++allocations;return std::make_unique<Images>();});PreparedSceneGpuBridgeCandidate<Images>freshBridge(freshCache);try{(void)freshBridge.resources(scene,VkBuffer(1),VkBuffer(2),3*sizeof(SceneGpuVertexCandidate),12,VkPipeline(10),VkPipeline(20),VkPipelineLayout(1),std::nullopt,VkPipeline(30));}catch(const std::runtime_error&){++rejected;if(allocations)throw std::runtime_error("unsupported material reached GPU factory");return;}throw std::runtime_error("unsupported material accepted");};
 material.additive=false;reject();material=saved;
 material.secondBaseImage={};reject();material=saved;
 scene.materialAlpha[1]=.5f;reject();scene.materialAlpha[1]=1;
 material.source.syntax=parseVmtCandidate("UnlitTwoTexture {$cloakfactor .5}");reject();material=saved;
 material.source.syntax=parseVmtCandidate("UnlitTwoTexture {$color \"[1 .5 1]\"}");reject();material=saved;
 material.source.syntax=parseVmtCandidate("UnlitTwoTexture {$color bad}");reject();material=saved;
 material.source.syntax=parseVmtCandidate("UnlitTwoTexture {$color \"[1 1 1]\" $color \"[1 1 1]\"}");reject();material=saved;
 material.unlitTwoTexture=false;auto ordinary=generate(VkPipeline(30));if(ordinary.twoTextureSlots!=std::vector<uint8_t>{0,0})throw std::runtime_error("ordinary frame rejected");material=saved;
 for(auto text:{"UnlitTwoTexture {$color \"[1 1 1\"}","UnlitTwoTexture {$color \"1 1 1]\"}","UnlitTwoTexture {$color \"[1 [1] 1]\"}","UnlitTwoTexture {$color \"[1 1 1] junk\"}"}){material.source.syntax=parseVmtCandidate(text);reject();material=saved;}
 material.source.baseTexture2Missing=true;reject();material=saved;
 material.image.rgba.pop_back();reject();material=saved;
 material.secondTextureTransform[0]=std::numeric_limits<float>::infinity();reject();material=saved;
 if(rejected!=14)throw std::runtime_error("rejection coverage");
 std::cout<<"PASS: bridge material identity, independent UV, legacy default and 14 unsupported inputs rejected before GPU allocation plus ordinary frames; CPU resources only\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
