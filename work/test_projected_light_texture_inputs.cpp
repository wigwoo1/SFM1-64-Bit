#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "film_scene_render_binding_candidate.h"
#include "projected_light_texture_inputs_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "scene_projected_light_inputs_candidate.h"
#include <iostream>
int main(){try{
 static_assert(sizeof(void*)==8);auto graph=loadGraph("work/short-timeline-session.dmx");auto binding=readFilmSceneRenderBindingCandidate(graph,2470);GraphTransformHierarchy hierarchy(graph);
 std::filesystem::path game=R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game)";
 std::ifstream f(game/"usermod/gameinfo.txt",std::ios::binary);if(!f)throw std::runtime_error("gameinfo missing");std::string text((std::istreambuf_iterator<char>(f)),{});auto mounts=gameinfoMountsCandidate(text,game/"usermod",game);
 ProjectedLightTextureInputsCandidate cache;
 std::vector<SessionRenderLightCandidate> snapshots;for(auto id:binding.lights)snapshots.push_back(readSessionRenderLightCandidate(graph,id,hierarchy));
 auto bundle=prepareSceneProjectedLightInputsCandidate(snapshots,cache,mounts);if(bundle.size()!=3)throw std::runtime_error("light bundle coverage");
 for(size_t i=0;i<bundle.size();++i){auto&input=bundle[i];if(input.source.id!=binding.lights[i]||!input.requiresShadows||input.requiresUberlight||input.requiresVolumetric||input.constants.colorFar[0]<=1||input.constants.attenuationAmbient[2]!=1500||!input.texture.image)throw std::runtime_error("light controls lost");}
 auto duplicate=snapshots;duplicate.push_back(snapshots.front());bool duplicateRejected=false;try{prepareSceneProjectedLightInputsCandidate(duplicate,cache,mounts);}catch(const std::runtime_error&){duplicateRejected=true;}if(!duplicateRejected)throw std::runtime_error("duplicate bundle light");
 std::cout<<"three complete CPU light inputs retain HDR, attenuation, projection and shadow requirements; duplicate rejected\n";
 std::shared_ptr<const VtfRgbaImageCandidate>first;
 for(auto id:binding.lights){auto light=readSessionRenderLightCandidate(graph,id,hierarchy);auto texture=cache.prepare(light,mounts);if(!texture.image||texture.frame!=0||texture.mip!=0)throw std::runtime_error("texture selection");auto independent=decodeVtfMipCandidate(readBinaryAssetCandidate(texture.path),0,0);if(independent.rgba!=texture.image->rgba)throw std::runtime_error("decoded image mismatch");if(first&&first!=texture.image)throw std::runtime_error("shared gobo duplicated");first=texture.image;std::cout<<"light="<<id<<" texture="<<texture.path<<" size="<<first->width<<'x'<<first->height<<'\n';
  light.fields.textureName="";if(cache.prepare(light,mounts).image)throw std::runtime_error("empty texture policy");light.fields.textureName="effects/sfm64_missing_owned_fixture";bool rejected=false;try{cache.prepare(light,mounts);}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("missing texture accepted");
 }
 if(cache.assetCount()!=1||cache.imageCount()!=1)throw std::runtime_error("cache cardinality");std::cout<<"x64 three shot lights share one decoded gobo; empty/missing handling passed; GPU projection unverified\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
