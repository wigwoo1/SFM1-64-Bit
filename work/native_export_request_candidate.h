#pragma once
#include "win32_export_process_candidate.h"
#include "render_settings_candidate.h"
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <rpc.h>
#include <cstdint>
#pragma comment(lib,"Rpcrt4.lib")
inline std::string exportJsonStringCandidate(const std::string& value){
 const char hex[]="0123456789abcdef";std::string output="\"";
 for(unsigned char character:value){if(character=='"'||character=='\\'){output+='\\';output+=char(character);}else if(character<32){output+="\\u00";output+=hex[character>>4];output+=hex[character&15];}else output+=char(character);}
 return output+'"';
}
struct ExportConfigurationCandidate {
 std::filesystem::path python,backend,blender,sfm,converterSnapshot,cyclesSchema,requestRoot;
};
inline std::string readExportStatusCandidate(const std::filesystem::path&path){
 HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(file==INVALID_HANDLE_VALUE){if(GetLastError()==ERROR_FILE_NOT_FOUND||GetLastError()==ERROR_PATH_NOT_FOUND)return {};throw std::runtime_error("Export status read open failed");}
 char bytes[65536];DWORD length=0;BOOL read=ReadFile(file,bytes,sizeof(bytes),&length,nullptr);CloseHandle(file);
 if(!read||length==sizeof(bytes))throw std::runtime_error("Export status invalid or oversized");return std::string(bytes,length);
}
class NativeExportRequestCandidate {
 std::filesystem::path folder_;
 std::unique_ptr<ExportProcessCandidate> process_;
 uint64_t revision_=0;
 std::filesystem::path python_,acceptanceScript_,presentationFolder_;
 std::unique_ptr<ExportProcessCandidate> acceptanceProcess_;
public:
 NativeExportRequestCandidate(const ExportConfigurationCandidate& configuration,const PlaybackRenderSettingsCandidate& settings,
  const std::function<void(const std::filesystem::path&)>& capture,const std::string& scene={},uint64_t revision=0):revision_(revision){
  if(settings.renderer!=2)throw std::runtime_error("Only Cycles export dispatch is implemented");
  python_=configuration.python;acceptanceScript_=std::filesystem::absolute(configuration.backend).parent_path()/"export_result_acceptance_candidate.py";
  auto settingsJson=renderSettingsJsonCandidate(settings);
  if(!scene.empty()){settingsJson.pop_back();settingsJson+=",\"scene\":"+exportJsonStringCandidate(scene)+"}";}
  UUID uuid{};auto status=UuidCreate(&uuid);if(status!=RPC_S_OK&&status!=RPC_S_UUID_LOCAL_ONLY)throw std::runtime_error("Export identity creation failed");
  RPC_CSTR text=nullptr;if(UuidToStringA(&uuid,&text)!=RPC_S_OK)throw std::runtime_error("Export identity formatting failed");
  std::string identifier(reinterpret_cast<char*>(text));RpcStringFreeA(&text);
  std::filesystem::create_directories(configuration.requestRoot);
  folder_=std::filesystem::absolute(configuration.requestRoot/identifier.substr(0,8));
  if(!std::filesystem::create_directory(folder_))throw std::runtime_error("Export request folder already exists");
  auto session=folder_/"session.dmx";capture(session);
  if(!std::filesystem::is_regular_file(session)||!std::filesystem::file_size(session))throw std::runtime_error("Export capture missing");
  auto field=[](const char*name,const std::filesystem::path&path){return exportJsonStringCandidate(name)+":"+exportJsonStringCandidate(std::filesystem::absolute(path).u8string());};
  std::string request="{\"version\":1,\"editor_revision\":"+std::to_string(revision_)+",\"request_id\":"+exportJsonStringCandidate(identifier)+","+field("session",session)+","+field("sfm",configuration.sfm)+","+field("blender",configuration.blender)+","+field("converter_snapshot",configuration.converterSnapshot)+","+field("cycles_schema",configuration.cyclesSchema)+",\"settings\":"+settingsJson+"}";
  auto requestPath=folder_/"request.json";
  std::ofstream file(requestPath,std::ios::binary);file<<request;file.close();if(!file)throw std::runtime_error("Export request write failed");
  process_=std::make_unique<ExportProcessCandidate>(configuration.python.wstring(),std::vector<std::wstring>{std::filesystem::absolute(configuration.backend).wstring(),requestPath.wstring()});
 }
 NativeExportRequestCandidate(const NativeExportRequestCandidate&)=delete;
 NativeExportRequestCandidate&operator=(const NativeExportRequestCandidate&)=delete;
 std::optional<DWORD> poll()const{return process_->poll();}
 bool matchesRevision(uint64_t revision)const{return revision==revision_;}
 void startAcceptance(uint64_t currentRevision){
  if(!matchesRevision(currentRevision))throw std::runtime_error("Export belongs to an earlier document revision");
  auto code=poll();if(!code||*code!=0)throw std::runtime_error("Export has not finished successfully");
  if(acceptanceProcess_)throw std::runtime_error("Image acceptance already started");
  UUID uuid{};auto result=UuidCreate(&uuid);if(result!=RPC_S_OK&&result!=RPC_S_UUID_LOCAL_ONLY)throw std::runtime_error("Acceptance identity creation failed");
  RPC_CSTR text=nullptr;if(UuidToStringA(&uuid,&text)!=RPC_S_OK)throw std::runtime_error("Acceptance identity formatting failed");
  std::string identifier(reinterpret_cast<char*>(text));RpcStringFreeA(&text);
  presentationFolder_=folder_/"backend"/("accepted-"+identifier.substr(0,8));
  acceptanceProcess_=std::make_unique<ExportProcessCandidate>(python_.wstring(),std::vector<std::wstring>{acceptanceScript_.wstring(),(folder_/"request.json").wstring(),L"--observed-exit-code",L"0",L"--current-revision",std::to_wstring(currentRevision),L"--presentation-folder",presentationFolder_.wstring()});
 }
 std::optional<DWORD> acceptancePoll()const{return acceptanceProcess_?acceptanceProcess_->poll():std::optional<DWORD>{};}
 bool acceptanceStarted()const{return bool(acceptanceProcess_);}
 std::filesystem::path acceptedImage(uint64_t currentRevision)const{
  auto code=acceptancePoll();if(!matchesRevision(currentRevision)||!code||*code!=0)throw std::runtime_error("Accepted image is unavailable for current document");
  auto path=presentationFolder_/"render.png";if(!std::filesystem::is_regular_file(path))throw std::runtime_error("Accepted image is missing");return path;
 }
 const std::filesystem::path&folder()const{return folder_;}
 void cancel(){std::ofstream marker(folder_/"cancel",std::ios::binary);marker.close();if(!marker)throw std::runtime_error("Export cancellation request failed");}
};
