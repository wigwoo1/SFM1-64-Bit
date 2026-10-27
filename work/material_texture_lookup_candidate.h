#pragma once
#include "material_find_cache_key_candidate.h"
#include <functional>
#include <optional>
#include <cstdint>

// Texture-manager lookup keys differ from material keys: a final .hdr suffix is
// retained. Engine logical identity only, never host-filesystem authorization.
inline bool materialTextureCacheKeyCandidate(std::string_view input,std::string&output){
 if(input.size()>4095)return false;input=input.substr(0,input.find('\0'));
 if(input.size()>259)return false;for(unsigned char c:input)if(c>=128)return false;
 bool hdr=input.size()>4;const std::string_view suffix=".hdr";
 if(hdr)for(size_t i=0;i<4;++i){char c=input[input.size()-4+i];if(c>='A'&&c<='Z')c=char(c+32);if(c!=suffix[i])hdr=false;}
 std::string key;if(!materialFindCacheKeyCandidate(input,key))return false;if(hdr)key+=".hdr";output.swap(key);return true;
}
struct MaterialTextureLookupRequestCandidate {
 std::optional<std::string>name;
 std::optional<std::string>group=std::string();
 uint8_t complain=0;int32_t flags=0;
};
// BORROWED service results, not a texture owner, factory, VTF loader or GPU handle.
// nullopt is unavailable. A zero/sentinel return cannot be dereferenced by the
// original wrapper; refuse it rather than inventing successful null semantics.
struct MaterialTextureLookupServicesCandidate {
 std::function<std::optional<uintptr_t>(const std::string&,const std::optional<std::string>&,int32_t)>find;
 std::function<std::optional<bool>(uintptr_t)>isError;
};
struct MaterialTextureLookupResultCandidate {uintptr_t texture=0;bool warning=false,redirected=false;};
inline std::optional<MaterialTextureLookupResultCandidate>materialTextureLookupCandidate(
 const MaterialTextureLookupRequestCandidate&request,const MaterialTextureLookupServicesCandidate&services)noexcept{
 try{
  if(!request.name||!services.find||!services.isError||request.name->size()>4095||(request.group&&request.group->size()>4095))return {};
  auto name=*request.name;auto group=request.group;name.resize(name.find('\0')==std::string::npos?name.size():name.find('\0'));
  if(group)group->resize(group->find('\0')==std::string::npos?group->size():group->find('\0'));
  if(name.size()>259||(group&&group->size()>255))return {};for(unsigned char c:name)if(c>=128)return {};if(group)for(unsigned char c:*group)if(c>=128)return {};
  // Host callbacks may replace their source request/service table. Own the
  // in-flight callables and scalar arguments before invoking any of them.
  const auto find=services.find;const auto isError=services.isError;
  const auto flags=request.flags;const auto complain=request.complain;
  MaterialTextureLookupResultCandidate result;
  for(unsigned attempt=0;attempt<2;++attempt){
   auto token=find(name,group,flags);if(!token||!*token||*token==UINTPTR_MAX)return {};
   auto error=isError(*token);if(!error)return {};
   if(*error&&!attempt){std::string lower=name;for(char&c:lower)if(c>='A'&&c<='Z')c=char(c+32);if(lower=="_rt_fullframefb1"){name="_rt_FullScreen";result.redirected=true;continue;}}
   result.texture=*token;result.warning=*error&&complain!=0;return result;
  }
  return {};
 }catch(...){return {};}
}
