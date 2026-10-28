#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <filesystem>
#include "material_texture_file_resolver_candidate.h"

namespace mounted_material_texture_detail {
class Handle {
 HANDLE value=INVALID_HANDLE_VALUE;
public:
 explicit Handle(HANDLE h=INVALID_HANDLE_VALUE)noexcept:value(h){}
 ~Handle(){if(value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
 Handle(const Handle&)=delete;Handle&operator=(const Handle&)=delete;
 Handle(Handle&&other)noexcept:value(other.value){other.value=INVALID_HANDLE_VALUE;}
 HANDLE get()const noexcept{return value;}
 explicit operator bool()const noexcept{return value!=INVALID_HANDLE_VALUE;}
};
inline bool absent(DWORD error)noexcept{return error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND;}
inline std::wstring openPath(const std::filesystem::path&directory){
 auto name=directory.native();if(name.empty()||name.find(L'\0')!=std::wstring::npos)throw std::invalid_argument("mounted texture directory path");for(auto&c:name)if(c==L'/')c=L'\\';
 if(name.compare(0,4,L"\\\\?\\")==0)name.erase(0,4);
 if(name.size()<3||!((name[0]>=L'A'&&name[0]<=L'Z')||(name[0]>=L'a'&&name[0]<=L'z'))||name[1]!=L':'||name[2]!=L'\\'||name.find(L':',2)!=std::wstring::npos)throw std::invalid_argument("mounted texture requires absolute local DOS mount");
 while(name.size()>3&&name.back()==L'\\')name.pop_back();size_t start=3;
 while(start<name.size()){auto end=name.find(L'\\',start);auto part=std::wstring_view(name).substr(start,end==std::wstring::npos?end:end-start);if(part.empty()||part==L"."||part==L"..")throw std::invalid_argument("mounted texture mount must be normalized");if(end==std::wstring::npos)break;start=end+1;}
 return L"\\\\?\\"+name;
}
inline std::wstring finalPath(HANDLE h){
 // NT volume names avoid a DOS-volume mapping query, which can be unavailable
 // even for a readable local drive. Both root and file use the same normalized
 // representation, still resolved from their actual handles (never OPENED).
 const DWORD needed=GetFinalPathNameByHandleW(h,nullptr,0,FILE_NAME_NORMALIZED|VOLUME_NAME_NT);
 if(!needed||needed>=32767)throw std::runtime_error("mounted texture final path unavailable");
 std::wstring out(needed,L'\0');const DWORD size=GetFinalPathNameByHandleW(h,out.data(),needed,FILE_NAME_NORMALIZED|VOLUME_NAME_NT);
 if(!size||size>=needed)throw std::runtime_error("mounted texture final path changed");out.resize(size);
 const std::wstring_view prefix=L"\\Device\\HarddiskVolume";if(out.compare(0,prefix.size(),prefix))throw std::runtime_error("mounted texture requires local disk volume");size_t at=prefix.size();while(at<out.size()&&out[at]>=L'0'&&out[at]<=L'9')++at;if(at==prefix.size()||(at<out.size()&&out[at]!=L'\\'))throw std::runtime_error("mounted texture volume name unavailable");
 while(!out.empty()&&out.back()==L'\\')out.pop_back();return out;
}
inline bool sameIdentity(const BY_HANDLE_FILE_INFORMATION&a,const BY_HANDLE_FILE_INFORMATION&b)noexcept{
 return a.dwVolumeSerialNumber==b.dwVolumeSerialNumber&&a.nFileIndexHigh==b.nFileIndexHigh&&a.nFileIndexLow==b.nFileIndexLow;
}
}

// Both arguments must use the same normalized volume-path representation from
// open handles. The provider compares local NT volume paths.
// Exact canonical spelling intentionally refuses ambiguous case aliases instead
// of authorizing a differently cased sibling on a case-sensitive filesystem.
inline bool mountedMaterialTexturePathInsideCandidate(const std::wstring&file,const std::wstring&directory)noexcept{
 return !directory.empty()&&file.size()>directory.size()&&file.compare(0,directory.size(),directory)==0&&(directory.back()==L'\\'||file[directory.size()]==L'\\');
}

// Immutable, ordered loose GAME mounts; no cache, native filesystem emulation,
// VPK, GPU or writable-path concept. Missing mount directories are omitted at
// construction. Existing directory handles/identities and normalized paths are
// retained; a moved/deleted mount is unavailable rather than silently replaced.
// Handles allow sharing read/write/delete on directories (no directory locking).
// File reads share read/delete only, retain the selected open file through read,
// and validate that SAME handle's final path. Arbitrary concurrent namespace or
// memory-mapped content changes are not claimed to form an atomic snapshot.
class MountedMaterialTextureFileProviderCandidate {
 using Handle=mounted_material_texture_detail::Handle;
 struct Mount {
  Handle handle;std::wstring path,openPath;BY_HANDLE_FILE_INFORMATION identity{};
  Mount(Handle h,std::wstring p,std::wstring name,BY_HANDLE_FILE_INFORMATION info):handle(std::move(h)),path(std::move(p)),openPath(std::move(name)),identity(info){}
  bool valid()const{
   BY_HANDLE_FILE_INFORMATION info{};
   return GetFileInformationByHandle(handle.get(),&info)&&(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&mounted_material_texture_detail::sameIdentity(identity,info)&&mounted_material_texture_detail::finalPath(handle.get())==path;
  }
 };
 struct State {
  std::vector<Mount>mounts;uint64_t limit;
  State(const std::vector<std::filesystem::path>&input,uint64_t maxBytes):limit(maxBytes){
   if(!limit||limit>(uint64_t(1)<<29))throw std::invalid_argument("mounted texture size limit");
   for(const auto&directory:input){auto name=mounted_material_texture_detail::openPath(directory);
    Handle handle(CreateFileW(name.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr));
    if(!handle){if(mounted_material_texture_detail::absent(GetLastError()))continue;throw std::runtime_error("mounted texture directory unavailable");}
    BY_HANDLE_FILE_INFORMATION info{};if(GetFileType(handle.get())!=FILE_TYPE_DISK||!GetFileInformationByHandle(handle.get(),&info)||!(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))throw std::runtime_error("mounted texture directory required");
    auto canonical=mounted_material_texture_detail::finalPath(handle.get());mounts.emplace_back(std::move(handle),std::move(canonical),std::move(name),info);
   }
  }
  std::optional<std::vector<uint8_t>>read(const MaterialTextureFileRequestCandidate&request)const noexcept{
   try{
    MaterialTextureFileRequestCandidate checked;
    if(!materialTextureFileRequestCandidate(request.key,checked)||request.key!=checked.key||request.file!=checked.file||request.pathID!=checked.pathID||request.mode!=checked.mode)return {};
    std::wstring relative(request.file.begin(),request.file.end());for(auto&c:relative)if(c==L'/')c=L'\\';
    for(const auto&mount:mounts){
     if(!mount.valid())return {};auto path=mount.openPath;if(path.back()!=L'\\')path+=L'\\';path+=relative;if(path.size()>=32767)return {};
     Handle file(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,nullptr));
     if(!file){if(mounted_material_texture_detail::absent(GetLastError()))continue;return {};}
     BY_HANDLE_FILE_INFORMATION info{};LARGE_INTEGER size{};
     if(GetFileType(file.get())!=FILE_TYPE_DISK||!GetFileInformationByHandle(file.get(),&info)||(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)||!GetFileSizeEx(file.get(),&size)||size.QuadPart<0||uint64_t(size.QuadPart)>limit)return {};
     if(!mountedMaterialTexturePathInsideCandidate(mounted_material_texture_detail::finalPath(file.get()),mount.path)||!mount.valid())return {};
     std::vector<uint8_t>bytes(static_cast<size_t>(size.QuadPart));size_t offset=0;
     while(offset<bytes.size()){const DWORD count=static_cast<DWORD>((std::min)(bytes.size()-offset,size_t(1)<<20));DWORD received=0;if(!ReadFile(file.get(),bytes.data()+offset,count,&received,nullptr)||!received||received>count)return {};offset+=received;}
     uint8_t extra=0;DWORD count=0;LARGE_INTEGER after{};
     if(!ReadFile(file.get(),&extra,1,&count,nullptr)||count||!GetFileSizeEx(file.get(),&after)||after.QuadPart!=size.QuadPart||!mount.valid())return {};
     return bytes; // No fallback after selecting a present file, even invalid VTF.
    }
   }catch(...){return {};}
   return {};
  }
 };
 std::shared_ptr<const State>state;
public:
 explicit MountedMaterialTextureFileProviderCandidate(const std::vector<std::filesystem::path>&mounts,uint64_t maxBytes=uint64_t(1)<<29):state(std::make_shared<State>(mounts,maxBytes)){}
 MaterialTextureFileResolverCandidate::Provider provider()const{const auto retained=state;return [retained](const auto&request){return retained->read(request);};}
};
