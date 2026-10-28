#pragma once
#include "material_texture_resource_candidate.h"
#include <map>

struct MaterialTextureFileRequestCandidate {std::string key,file,pathID="GAME",mode="rb";};
// Explicit safe loose-file subset. The existing texture lookup key rules remain
// unchanged; this adds file-path admission and is not a native texture-manager
// LoadVTF reconstruction. Providers must authorize their own mounted paths.
inline bool materialTextureFileRequestCandidate(std::string_view name,MaterialTextureFileRequestCandidate&out){
 if(name.find('\0')!=std::string_view::npos)return false;
 MaterialTextureFileRequestCandidate next;
 if(!materialTextureCacheKeyCandidate(name,next.key)||next.key.empty()||next.key.front()=='/'||next.key.compare(0,4,"_rt_")==0||next.key=="env_cubemap")return false;
 for(unsigned char c:next.key)if(c<32||c==127||c==':'||c=='*'||c=='?'||c=='"'||c=='<'||c=='>'||c=='|')return false;
 size_t start=0;
 for(;;){auto end=next.key.find('/',start);auto part=std::string_view(next.key).substr(start,end==std::string::npos?end:end-start);
  if(part.empty()||part=="."||part==".."||part.back()=='.'||part.back()==' ')return false;
  auto stem=part.substr(0,part.find('.'));
  if(stem=="con"||stem=="prn"||stem=="aux"||stem=="nul"||(stem.size()==4&&(stem.substr(0,3)=="com"||stem.substr(0,3)=="lpt")&&stem[3]>='1'&&stem[3]<='9'))return false;
  if(end==std::string::npos)break;start=end+1;
 }
 next.file="materials/"+next.key+".vtf";
 if(next.file.size()>=260)return false;
 out=std::move(next);return true;
}

// Owner-thread, opt-in CPU VTF service. Provider returns a complete read-only
// byte snapshot for an admitted logical GAME request. Only successfully copied
// and validated files enter this owner-scoped cache; failures are retryable.
// Cache identity/contents stay fixed for this provider lifetime. Create a new
// service to reload a different mount/file version; no eviction or GPU is here.
// A transaction may retain successful cache reads even when its values roll
// back. Returned typed references independently retain their immutable bytes.
class MaterialTextureFileResolverCandidate {
public:
 using Provider=std::function<std::optional<std::vector<uint8_t>>(const MaterialTextureFileRequestCandidate&)>;
private:
 struct State {
  Provider provider;std::map<std::string,MaterialTextureReferenceCandidate>cache;bool busy=false;
  explicit State(Provider p):provider(std::move(p)){}
  std::optional<MaterialTextureReferenceCandidate>resolve(const std::optional<std::string>&name)noexcept{
   if(busy||!name)return {};
   busy=true;struct Guard{bool&b;~Guard(){b=false;}}guard{busy};
   try{
    // env_cubemap is the existing typed sentinel, never a fabricated file or
    // scene cubemap image. Render-target names remain unavailable here.
    std::string key;if(name->find('\0')!=std::string::npos||!materialTextureCacheKeyCandidate(*name,key))return {};
    if(key=="env_cubemap")return MaterialTextureReferenceCandidate::environment();
    MaterialTextureFileRequestCandidate request;if(!materialTextureFileRequestCandidate(*name,request))return {};
    auto found=cache.find(request.key);if(found!=cache.end())return found->second;
    if(!provider)return {};const auto read=provider;auto bytes=read(request);if(!bytes)return {};
    auto value=MaterialTextureReferenceCandidate::ordinary(MaterialTextureResourceCandidate::fromVtf(request.key,*bytes));
    auto inserted=cache.emplace(request.key,value);return inserted.first->second;
   }catch(...){return {};}
  }
 };
 std::shared_ptr<State>state;
public:
 explicit MaterialTextureFileResolverCandidate(Provider provider):state(std::make_shared<State>(std::move(provider))){}
 MaterialTextureFileResolverCandidate(const MaterialTextureFileResolverCandidate&)=delete;
 MaterialTextureFileResolverCandidate&operator=(const MaterialTextureFileResolverCandidate&)=delete;
 std::optional<MaterialTextureReferenceCandidate>resolve(const std::optional<std::string>&name)const noexcept{const auto retained=state;return retained->resolve(name);}
 MaterialTextureResolverCandidate resolver()const{const auto retained=state;return [retained](const auto&name){return retained->resolve(name);};}
 size_t cachedCount()const noexcept{return state->cache.size();}
};
