#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <atomic>
// Same-volume commit without replacement; original session is never opened.
inline void commitSessionSaveWithoutReplacementCandidate(const std::filesystem::path&stage,const std::filesystem::path&target){
 // MoveFileEx without MOVEFILE_REPLACE_EXISTING is the final atomic
 // no-replace check. A target created after staging therefore makes commit fail.
 if(!MoveFileExA(stage.string().c_str(),target.string().c_str(),MOVEFILE_WRITE_THROUGH))
  throw std::runtime_error("save commit failed; destination may already exist");
}
inline void atomicSessionSaveAsCandidate(const Graph&graph,const char*path){
 std::filesystem::path target(path);if(target.empty()||std::filesystem::exists(target))throw std::runtime_error("save destination exists or invalid");
 auto parent=target.parent_path();if(parent.empty())parent=".";if(!std::filesystem::is_directory(parent))throw std::runtime_error("save destination directory missing");
 static std::atomic<uint64_t>serial{0};std::filesystem::path directory;bool created=false;
 for(unsigned attempt=0;attempt<64;++attempt){directory=parent/(".sfm64-save-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64())+"-"+std::to_string(serial.fetch_add(1)));if(std::filesystem::create_directory(directory)){created=true;break;}}
 if(!created)throw std::runtime_error("save staging directory unavailable");auto stage=directory/"session.dmx";
 auto cleanup=[&]{std::error_code ignored;std::filesystem::remove(stage,ignored);std::filesystem::remove(directory,ignored);};
 try{saveGraph(graph,stage.string().c_str());HANDLE file=CreateFileA(stage.string().c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE)throw std::runtime_error("save staging flush open failed");BOOL flushed=FlushFileBuffers(file);CloseHandle(file);if(!flushed)throw std::runtime_error("save staging flush failed");commitSessionSaveWithoutReplacementCandidate(stage,target);cleanup();}catch(...){cleanup();throw;}
}
