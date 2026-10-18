#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "projected_light_saved_fields_candidate.h"
#include "film_scene_render_binding_candidate.h"
#include "session_render_light_candidate.h"
#include "channel_binding.h"
#include "projected_light_prepared_fields_candidate.h"
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8);if(argc!=2)throw std::runtime_error("session required");
 auto graph=loadGraph(argv[1]);size_t count=0;
 std::set<uint32_t> boundLights;size_t shots=0;GraphTransformHierarchy hierarchy(graph);
 for(uint32_t id=0;id<graph.elements.size();++id){auto& e=graph.elements[id];if(graph.strings.at(e.type)!="DmeFilmClip"||hierarchyRefs(graph,e,"camera",1).empty())continue;
  auto binding=readFilmSceneRenderBindingCandidate(graph,id);std::set<uint32_t> unique(binding.lights.begin(),binding.lights.end());if(unique.size()!=binding.lights.size())throw std::runtime_error("duplicate bound light");
  std::cout<<"shot="<<id<<" bound_lights="<<binding.lights.size()<<" light_ids=";for(auto light:binding.lights){if(graph.strings.at(graph.elements.at(light).type)!="DmeProjectedLight")throw std::runtime_error("light binding type");auto snapshot=readSessionRenderLightCandidate(graph,light,hierarchy);if(snapshot.id!=light)throw std::runtime_error("snapshot identity");boundLights.insert(light);std::cout<<light<<',';}std::cout<<'\n';++shots;
 }
 std::set<uint32_t> lightTargets=boundLights;
 for(auto id:boundLights)lightTargets.insert(projectedLightSavedFieldsCandidate(graph,id).transformReference);
 size_t lightChannels=0,varyingLayers=0;
 for(uint32_t id=0;id<graph.elements.size();++id){auto&e=graph.elements[id];if(graph.strings.at(e.type)!="DmeChannel")continue;
  auto endpoint=resolveEndpoint(graph,e,"to");if(!endpoint.present()||!lightTargets.count(endpoint.element))continue;
  ChannelBinding channel(graph,id);size_t varying=0,keys=0;int32_t start=INT32_MAX,end=INT32_MIN;
  for(auto layer:channel.layers){auto&le=graph.elements.at(layer);auto* values=namedAttribute(graph,le,"values");auto* times=namedAttribute(graph,le,"times");if(values){keys+=values->values.size();std::set<std::vector<char>> words;for(auto&v:values->values)words.insert(std::vector<char>(v.wire.begin(),v.wire.end()));if(words.size()>1){++varying;++varyingLayers;}}if(times)for(auto&v:times->values){auto t=wireInt(v);start=std::min(start,t);end=std::max(end,t);}}
  std::cout<<"light_channel="<<id<<" target="<<endpoint.element<<" field="<<endpoint.attribute<<" mode="<<channel.mode<<" keys="<<keys<<" varying_layers="<<varying<<" log_time_range="<<start<<','<<end<<'\n';++lightChannels;
 }
 std::cout<<"light_target_channels="<<lightChannels<<" varying_light_layers="<<varyingLayers<<'\n';
 std::cout<<"camera_bound_shots="<<shots<<" unique_bound_lights="<<boundLights.size()<<'\n';
 for(size_t i=0;i<graph.elements.size();++i){
  if(graph.strings.at(graph.elements[i].type)!="DmeProjectedLight")continue;
  auto saved=projectedLightSavedFieldsCandidate(graph,i);
  std::array<uint32_t,7> localPose{};
  std::copy(saved.localOrientationWords.begin(),saved.localOrientationWords.end(),localPose.begin());
  std::copy(saved.localPositionWords.begin(),saved.localPositionWords.end(),localPose.begin()+4);
  auto prepared=projectedLightPreparedFieldsCandidate(saved,localPose);
  auto scalar=[](uint32_t word){float v;std::memcpy(&v,&word,4);return v;};
  std::cout<<"light_index="<<i<<" rgb="<<scalar(prepared.rgbWords[0])<<','<<scalar(prepared.rgbWords[1])<<','<<scalar(prepared.rgbWords[2])
   <<" fov="<<scalar(prepared.controlWords[1])<<" attenuation="<<scalar(prepared.controlWords[27])<<','<<scalar(prepared.controlWords[28])<<','<<scalar(prepared.controlWords[29])
   <<" shadows="<<unsigned(saved.booleanBytes[0])<<" uberlight="<<unsigned(saved.booleanBytes[2])<<" volumetric="<<unsigned(saved.booleanBytes[3])<<" texture="<<saved.textureName<<'\n';++count;
 }
 std::cout<<"saved_projected_lights="<<count<<" scope=saved_fields_only animated_world_pose_and_GPU_integration=unfinished\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
