#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "retained_shot_timeline_candidate.h"
#include "film_scene_render_binding_candidate.h"
#include "session_model_placement_candidate.h"
#include "session_camera_view_projection_candidate.h"
#include "session_bone_pose_candidate.h"
#include <charconv>
#include <iostream>
template<class T>T number(const char*s){T v{};auto end=s+std::strlen(s);auto r=std::from_chars(s,end,v);if(r.ec!=std::errc{}||r.ptr!=end)throw std::runtime_error("invalid numeric argument");return v;}
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");if(argc<8)throw std::runtime_error("session root-clip content-root fresh-output scalar-workers vector-workers ticks... required");
 auto output=std::filesystem::path(argv[4]);if(std::filesystem::exists(output))throw std::runtime_error("refuse output overwrite");
 auto root=std::filesystem::weakly_canonical(argv[3]);auto scalar=number<size_t>(argv[5]),vector=number<size_t>(argv[6]);if(scalar>64||vector>64||scalar+vector>64)throw std::runtime_error("sampling worker budget");
 std::vector<int32_t>ticks;for(int i=7;i<argc;++i)ticks.push_back(number<int32_t>(argv[i]));auto saved=loadGraph(argv[1]);auto rootClip=number<uint32_t>(argv[2]);for(auto tick:ticks)selectFilmShotCandidate(saved,rootClip,tick);
 RetainedShotTimelineCandidate timeline(std::move(saved),rootClip);
 std::map<std::string,std::vector<GeometryBoneCandidate>>skeletons;
 auto write=[](const std::filesystem::path&p,const void*data,size_t size){std::ofstream f(p,std::ios::binary);f.write(static_cast<const char*>(data),std::streamsize(size));f.close();if(!f)throw std::runtime_error("frame input write");};
 for(size_t frame=0;frame<ticks.size();++frame){auto step=timeline.step(ticks[frame],scalar,vector);const auto&g=timeline.graph();auto binding=readFilmSceneRenderBindingCandidate(g,step.shot.clip);auto camera=sessionCameraViewProjectionCandidate(g,binding.camera,512,512,true);GraphTransformHierarchy hierarchy(g);auto directory=output/("frame-"+std::to_string(frame));std::filesystem::create_directories(directory);write(directory/"camera.bin",&camera,64);std::ofstream manifest(directory/"manifest.txt");manifest<<"tick="<<ticks[frame]<<" clip="<<step.shot.clip<<" local_tick="<<step.shot.localTick<<" rebuilt="<<step.rebuilt<<" executed="<<step.executed<<" scalar_jobs="<<step.scalarJobs<<" vector_jobs="<<step.vectorJobs<<" native_scheduler=unfinished\n";
  for(size_t n=0;n<binding.models.size();++n){auto id=binding.models[n];auto placement=readSessionModelPlacementCandidate(g,id,hierarchy);auto name=placement.asset;for(char&c:name)if(c=='\\')c='/';std::filesystem::path relative(name);if(relative.is_absolute()||relative.has_root_name()||relative.has_root_directory()||name.find(':')!=std::string::npos)throw std::runtime_error("model asset path");for(auto&part:relative)if(part=="..")throw std::runtime_error("model traversal");
   auto found=skeletons.find(name);if(found==skeletons.end()){auto path=std::filesystem::weakly_canonical(root/relative);auto inside=path.lexically_relative(root);if(inside.empty()||*inside.begin()=="..")throw std::runtime_error("model outside content root");std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("model asset missing");std::vector<uint8_t>bytes((std::istreambuf_iterator<char>(file)),{});found=skeletons.emplace(name,readGeometrySkeletonCandidate(bytes)).first;}
   auto locals=readSessionBonePoseCandidate(g,id,found->second);evaluateGeometryPosePaletteCandidate(found->second,locals);auto prefix="model-"+std::to_string(n);write(directory/(prefix+"-placement.bin"),&placement.matrix,64);uint32_t count=uint32_t(locals.size());std::vector<uint8_t>pose(4+locals.size()*48);std::memcpy(pose.data(),&count,4);std::memcpy(pose.data()+4,locals.data(),locals.size()*48);write(directory/(prefix+"-pose.bin"),pose.data(),pose.size());manifest<<prefix<<" element="<<id<<" asset="<<placement.asset<<" bones="<<count<<'\n';
  }manifest.close();if(!manifest)throw std::runtime_error("manifest write");std::cout<<"frame="<<frame<<" tick="<<ticks[frame]<<" clip="<<step.shot.clip<<" rebuilt="<<step.rebuilt<<" models="<<binding.models.size()<<'\n';
 }std::cout<<"retained shot sequence export passed frames="<<ticks.size()<<" skeleton_cache="<<skeletons.size()<<" native_full_parity=unverified\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
