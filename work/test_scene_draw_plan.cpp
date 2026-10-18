#include "scene_draw_plan_candidate.h"
#include <iostream>
struct Model {struct {bool visible=true;}model;std::vector<float>vertices;};
int main(){try{
 std::vector<Model>models{{{true},{1,1,1,3,3,3}},{{true},{4,4,4}}};
 std::vector<ModelRenderInputsCandidate>inputs(2);
 for(auto&input:inputs){input.materials.resize(2);input.materials[1].translucent=true;input.indices={0,1,2};input.ranges={{0,3,0},{0,3,1},{3,0,0},{3,0,1}};}
 auto depth=[](float value){return double(value);};
 auto plan=sceneDrawPlanCandidate(models,inputs,depth);
 if(plan.opaque.size()!=2||plan.opaque[0].model!=0||plan.opaque[1].model!=1||plan.translucent.size()!=2||plan.translucent[0].model!=1||plan.translucent[1].model!=0)throw std::runtime_error("pass ordering");
 models[1].model.visible=false;plan=sceneDrawPlanCandidate(models,inputs,depth);
 if(plan.opaque.size()!=1||plan.translucent.size()!=1)throw std::runtime_error("visibility");
 inputs[0].ranges[0].materialIndex=9;bool rejected=false;
 try{sceneDrawPlanCandidate(models,inputs,depth);}catch(const std::runtime_error&){rejected=true;}
 if(!rejected)throw std::runtime_error("opaque material accepted");
 inputs[0].ranges[0].materialIndex=0;inputs[0].indices[0]=999;rejected=false;
 try{sceneDrawPlanCandidate(models,inputs,depth);}catch(const std::runtime_error&){rejected=true;}
 if(!rejected)throw std::runtime_error("opaque vertex accepted");
 std::cout<<"scene draw plan passed opaque_then_transparent=1 global_transparency=1 hidden_excluded=1 empty_excluded=1 invalid_opaque_rejected=1 GPU_integration=unfinished\n";
 return 0;
 }catch(const std::exception&e){std::cerr<<e.what();return 1;}}
