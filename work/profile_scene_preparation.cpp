#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "retained_session_scene_candidate.h"
#include "gameinfo_mounts_candidate.h"
int main(){try{
 std::filesystem::path content=R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod)";std::ifstream file(content/"gameinfo.txt",std::ios::binary);std::string text((std::istreambuf_iterator<char>(file)),{});auto mounts=gameinfoMountsCandidate(text,content,content.parent_path());
 std::ofstream csv("outputs/sfm64-preparation-stage-profile.csv");csv<<"workers,frame,evaluation_ms,poses_ms,geometry_ms,materials_ms,upload_ms\n";
 for(size_t workers:{size_t(1),size_t(4)}){RetainedSessionSceneCandidate scene(loadGraph("work/roundtrip-11.dmx"),2237,mounts,workers,true,true);for(int n=0;n<180;++n){auto frame=scene.sample(1259500+(n%61)*10000/60);const auto&p=frame.profile;if(frame.materialInputs.size()!=4||frame.upload.vertices.size()!=110396)throw std::runtime_error("profile workload changed");csv<<workers<<','<<n<<','<<p.evaluation<<','<<p.poses<<','<<p.geometry<<','<<p.materials<<','<<p.upload<<'\n';}}
 if(!csv)throw std::runtime_error("profile output failed");std::cout<<"preparation stage profile passed 360 samples four models one/four workers\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
