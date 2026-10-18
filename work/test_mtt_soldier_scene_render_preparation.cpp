#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "retained_session_scene_candidate.h"
#include "session_render_root_candidate.h"
#include "session_timeline_lane_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "session_camera_frustum_guide_candidate.h"
#include <iostream>
#include <cstring>

int main(int argc,char**argv){
 try{
  if(argc!=3)throw std::runtime_error("usage: test_mtt_soldier_scene_render_preparation session.dmx gameinfo.txt");
  if(sessionCameraFrustumGuidesEnabledCandidate(false,nullptr,0)||!sessionCameraFrustumGuidesEnabledCandidate(true,nullptr,0)||!sessionCameraFrustumGuidesEnabledCandidate(false,"1",1)||sessionCameraFrustumGuidesEnabledCandidate(true,"0",1))throw std::runtime_error("camera-frustum UI/diagnostic override resolution mismatch");bool invalidOverride=false;try{(void)sessionCameraFrustumGuidesEnabledCandidate(true,"yes",3);}catch(const std::runtime_error&){invalidOverride=true;}if(!invalidOverride)throw std::runtime_error("invalid camera-frustum override accepted");
  auto graph=loadGraph(argv[1]);auto root=sessionRenderRootCandidate(graph);Graph graphWithoutGuides=graph;auto timelineFrames=sessionTimelineFramesCandidate(graph,root);auto timelineSegments=sessionTimelineLaneCandidate(graph,root,timelineFrames);bool benchmarkShotVisible=false;for(const auto&segment:timelineSegments)if(segment.startFrame<=119&&119<segment.endFrame)benchmarkShotVisible=true;if(timelineSegments.empty()||!benchmarkShotVisible)throw std::runtime_error("mtt_soldier timeline shot lane missing");
  auto bytes=readBinaryAssetCandidate(argv[2]);std::string gameinfo(bytes.begin(),bytes.end());
  auto content=std::filesystem::path(argv[2]).parent_path();auto mounts=gameinfoMountsCandidate(gameinfo,content,content.parent_path());
  const auto binding=readFilmSceneRenderBindingCandidate(graph,selectFilmShotCandidate(graph,root,49583).clip);
  if(binding.cameras.size()!=1||binding.cameras.front()!=binding.camera)throw std::runtime_error("benchmark active scene camera scope changed");
  RetainedSessionSceneCandidate scene(std::move(graph),root,mounts,4,true,true);size_t cameraGuides=0;
  scene.addSceneDecorationCandidate([&](const Graph&g,const SessionRenderSceneCandidate&s,const auto&,const auto&,PreparedSessionSceneCandidate&prepared){cameraGuides=appendSessionCameraFrustumGuidesCandidate(g,s,prepared,true);});
  auto frame=scene.sample(49583);const uint64_t triangles=frame.upload.indices.size()/3;
  RetainedSessionSceneCandidate sceneWithoutGuides(std::move(graphWithoutGuides),root,mounts,4,true,true);sceneWithoutGuides.addSceneDecorationCandidate([](const Graph&g,const SessionRenderSceneCandidate&s,const auto&,const auto&,PreparedSessionSceneCandidate&prepared){appendSessionCameraFrustumGuidesCandidate(g,s,prepared,false);});auto frameWithoutGuides=sceneWithoutGuides.sample(49583);if(frameWithoutGuides.upload.vertices.size()!=52497||frameWithoutGuides.upload.indices.size()/3!=36822)throw std::runtime_error("disabled camera-frustum setting still changed Vulkan scene geometry");
  if(cameraGuides!=0||frame.upload.vertices.size()!=52497||triangles!=36822||scene.assetCount()!=3||scene.materialCount()!=3||frame.materialInputs.at(3).materials.size()!=59||frame.upload.opaque.empty())throw std::runtime_error("mtt_soldier camera-guide geometry preparation mismatch guides="+std::to_string(cameraGuides)+" vertices="+std::to_string(frame.upload.vertices.size())+" triangles="+std::to_string(triangles)+" world_materials="+std::to_string(frame.materialInputs.size()>3?frame.materialInputs.at(3).materials.size():0));
  for(const auto&material:frame.materialInputs.at(3).materials)if(!material.vertexLighting)throw std::runtime_error("BSP world material missing vertex-lightmap shader flag");size_t transitionMaterials=0;for(const auto&material:frame.materialInputs.at(3).materials)if(vmtAsciiLowerCandidate(material.source.syntax.key)=="worldvertextransition"){++transitionMaterials;if(!material.vertexTransition)throw std::runtime_error("BSP WorldVertexTransition draw flag missing");}if(transitionMaterials!=2)throw std::runtime_error("BSP WorldVertexTransition material count mismatch");bool variedLight=false;size_t uploadedBlendOutOfRange=0;for(size_t i=0;i<42680;++i){const auto&vertex=frame.upload.vertices.at(9817+i);for(float value:vertex.normal)if(!std::isfinite(value)||value<0)throw std::runtime_error("invalid uploaded BSP vertex light");if(i&&vertex.normal!=frame.upload.vertices.at(9817).normal)variedLight=true;if(!std::isfinite(vertex.displacementAlpha))throw std::runtime_error("nonfinite uploaded BSP displacement alpha");if(vertex.displacementAlpha<0.f||vertex.displacementAlpha>255.f)++uploadedBlendOutOfRange;}if(!variedLight)throw std::runtime_error("BSP world vertex lightmap data did not vary");if(uploadedBlendOutOfRange!=55)throw std::runtime_error("BSP displacement alpha upload mismatch");
  if(frame.materialInputs.at(2).materials.size()!=1)throw std::runtime_error("BSP inline material slot count mismatch");auto bspMaterialPath=frame.materialInputs.at(2).materials[0].source.vmt.filename().string();std::transform(bspMaterialPath.begin(),bspMaterialPath.end(),bspMaterialPath.begin(),[](unsigned char c){return char(std::tolower(c));});if(bspMaterialPath!="cobweb001.vmt")throw std::runtime_error("BSP inline material resolution mismatch: "+bspMaterialPath);
  const auto expectedGuideCount=cameraGuides;auto repeat=scene.sample(49583);
  if(cameraGuides!=expectedGuideCount||frame.upload.indices!=repeat.upload.indices||frame.upload.vertices.size()!=repeat.upload.vertices.size())throw std::runtime_error("repeat benchmark camera-guide mesh addressing changed");
  for(size_t n=0;n<frame.upload.vertices.size();++n)if(std::memcmp(&frame.upload.vertices[n],&repeat.upload.vertices[n],sizeof(SceneGpuVertexCandidate)))throw std::runtime_error("repeat benchmark mesh data changed");
  if(timelineFrames.size()<=120)throw std::runtime_error("mtt_soldier timeline has no frame 120");
  const auto frame120Tick=timelineFrames.tick(120);auto frame120=scene.sample(frame120Tick);
  size_t pilotMaterials=0;for(size_t group=0;group<frame120.materialInputs.size();++group)for(const auto&material:frame120.materialInputs.at(group).materials)if(material.unlitTwoTexture){
   ++pilotMaterials;if(!material.additive||material.translucent||material.vertexTransition||material.image.width!=16||material.secondBaseImage.width!=32||material.source.baseTexture2.filename()!="pilotlight_motion.vtf")throw std::runtime_error("benchmark pilot two-texture inputs mismatch");
   if(std::abs(material.baseTextureTransform[2]-.003242f)>1e-6f||std::abs(material.baseTextureTransform[6]+.015513f)>1e-6f||std::abs(material.secondTextureTransform[2]-.719841f)>1e-6f||std::abs(material.secondTextureTransform[6]-.177722f)>1e-6f)throw std::runtime_error("benchmark saved pilot transforms mismatch");
  }if(pilotMaterials!=1)throw std::runtime_error("benchmark pilot material count");

  constexpr size_t animatedModelVertices=52497-42680;
  if(frame120.shot!=frame.shot||frame120.upload.vertices.size()<animatedModelVertices)throw std::runtime_error("mtt_soldier frame 120 changed shot or omitted model mesh");
  size_t changedAnimatedVertices=0;
  for(size_t n=0;n<animatedModelVertices;++n)if(std::memcmp(&frame.upload.vertices[n],&frame120.upload.vertices[n],sizeof(SceneGpuVertexCandidate)))++changedAnimatedVertices;
  if(!changedAnimatedVertices)throw std::runtime_error("mtt_soldier frame 119-to-120 animation produced no changed model vertices");
  std::cout<<"mtt_first_shot_prepared=1 models=3 camera_frustum_guides="<<cameraGuides<<" disabled_guides_vertices=52497 triangles=36822 ui_override_cases=4 invalid_override_rejected=1 world_vertices=42680 world_triangles=27147 vertices="<<frame.upload.vertices.size()<<" triangles="<<triangles<<" world_materials=59 world_vertex_transition_materials=2 displacement_alpha_values=42680 out_of_range_raw_alpha="<<uploadedBlendOutOfRange<<" bsp_inline_triangles=4 material=cobweb001.vmt timeline_segments="<<timelineSegments.size()<<" benchmark_frame_119_in_lane=1 repeat_geometry_exact=1 frame_120_tick="<<frame120Tick<<" animated_vertices_changed="<<changedAnimatedVertices<<" native_animation_parity=unverified rendered_pixels=unverified\n";
  return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
