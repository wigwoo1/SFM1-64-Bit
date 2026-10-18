#define NOMINMAX
#include "native_export_request_candidate.h"
#include <iostream>
int main(){try{
 ExportConfigurationCandidate config;
 config.python=L"C:\\Users\\wigwo\\.cache\\codex-runtimes\\codex-primary-runtime\\dependencies\\python\\python.exe";
 config.backend="work/sfm_export_backend_candidate.py";config.blender=L"C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe";
 config.sfm=L"F:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game";
 config.converterSnapshot="work/cv/65e1f239";config.cyclesSchema="outputs/cycles-worker/dynamic-enum-result.json";config.requestRoot="work/ne";
 PlaybackRenderSettingsCandidate settings;settings.renderer=2;settings.samples=8;
 NativeExportRequestCandidate request(config,settings,[](const std::filesystem::path&path){std::filesystem::copy_file("outputs/obj-gpu-import/session-embedded-obj/resaved-368c7db4-b1e7-434c-8e35-a2c5cff520cb.dmx",path);});
 bool cancelled=false;auto deadline=GetTickCount64()+120000;
 while(!request.poll()){
  std::string content=readExportStatusCandidate(request.folder()/"backend/status.json");
  if(!cancelled&&content.find("\"state\": \"converting\"")!=std::string::npos){request.cancel();cancelled=true;std::cout<<"native_export_cancel_requested\n"<<std::flush;}
  if(GetTickCount64()>deadline){request.cancel();throw std::runtime_error("Native export cancellation timeout");}Sleep(100);
 }
 if(!cancelled||request.poll().value()!=0)throw std::runtime_error("Backend lifecycle failed");
 std::string content=readExportStatusCandidate(request.folder()/"backend/status.json");
 if(content.find("\"state\": \"cancelled\"")==std::string::npos)throw std::runtime_error("Missing cancelled terminal status");
 std::cout<<"PASS native request serialization, private snapshot, contained backend/Blender launch, nonblocking poll and cancellation folder="<<request.folder().string()<<'\n';
 return 0;
 }catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
