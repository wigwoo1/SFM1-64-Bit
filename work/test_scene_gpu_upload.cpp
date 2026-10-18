#include "scene_gpu_upload_candidate.h"
#include <iostream>
struct Model {struct {bool visible=true;}model;std::vector<SceneGpuVertexCandidate>vertices;};
int main(){try{
 std::vector<Model>models(3);for(size_t m=0;m<3;++m)for(size_t n=0;n<3;++n)models[m].vertices.push_back({{float(m),float(n),float(m+1)},{0,0,1},{0,0}});
 std::vector<ModelRenderInputsCandidate>inputs(3);
 for(auto&input:inputs){input.materials.resize(2);input.materials[1].translucent=true;input.indices={0,1,2};input.ranges={{0,3,0},{0,3,1}};}
 auto depth=[](const auto&v){return double(v.position[2]);};
 auto verify=[&](const auto&upload){for(const auto&draw:upload.opaque){auto address=upload.materials.at(draw.materialSlot);if(address.model!=draw.model||address.material!=draw.materialIndex||inputs[address.model].materials[address.material].translucent)throw std::runtime_error("opaque material address");}for(const auto&draw:upload.translucent){auto address=upload.materials.at(draw.materialSlot);if(address.model!=draw.model||address.material!=draw.materialIndex||!inputs[address.model].materials[address.material].translucent)throw std::runtime_error("transparent material address");}};
 auto upload=prepareSceneGpuUploadCandidate(models,inputs,depth);verify(upload);
 if(upload.materials.size()!=6||upload.vertices.size()!=9||upload.indices!=std::vector<uint32_t>{0,1,2,3,4,5,6,7,8}||upload.translucent[0].model!=2)throw std::runtime_error("full scene layout");
 models[1].model.visible=false;upload=prepareSceneGpuUploadCandidate(models,inputs,depth);verify(upload);
 if(upload.materials.size()!=4||upload.vertices.size()!=6||upload.indices!=std::vector<uint32_t>{0,1,2,3,4,5}||upload.opaque[1].model!=2||upload.opaque[1].materialSlot!=2||upload.translucent[0].materialSlot!=3)throw std::runtime_error("hidden middle rebasing");
 std::cout<<"scene GPU upload passed global_material_slots=1 cross_model_transparency=1 hidden_middle_rebase=1 vertex_stride=32 Vulkan_submission=unfinished\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what();return 1;}}
