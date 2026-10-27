#pragma once
#include "material_texture_lookup_candidate.h"
#include "vtf_texture_layout.h"
#include <memory>

// Immutable, validated CPU file contents. No engine pointer, GPU object or
// invented missing/error texture. Caller supplies bytes; this layer does no IO.
class MaterialTextureResourceCandidate {
 std::string key;
 std::vector<uint8_t>contents;
 VtfTextureLayoutCandidate description;
 MaterialTextureResourceCandidate(std::string n,std::vector<uint8_t>b,VtfTextureLayoutCandidate d):key(std::move(n)),contents(std::move(b)),description(std::move(d)){}
public:
 MaterialTextureResourceCandidate(const MaterialTextureResourceCandidate&)=delete;
 MaterialTextureResourceCandidate&operator=(const MaterialTextureResourceCandidate&)=delete;
 static std::shared_ptr<const MaterialTextureResourceCandidate>fromVtf(std::string_view name,const std::vector<uint8_t>&bytes){
  std::string key;if(!materialTextureCacheKeyCandidate(name,key)||key.empty()||bytes.size()>(1u<<29))throw std::runtime_error("owned texture name/extent unavailable");
  // Copy before validation so the caller retains no mutable alias to storage.
  auto owned=bytes;auto layout=readVtfTextureLayoutCandidate(owned);
  return std::shared_ptr<const MaterialTextureResourceCandidate>(new MaterialTextureResourceCandidate(std::move(key),std::move(owned),std::move(layout)));
 }
 const std::string&name()const noexcept{return key;}
 const std::vector<uint8_t>&bytes()const noexcept{return contents;}
 const VtfTextureLayoutCandidate&layout()const noexcept{return description;}
};
// Ordinary retained file resource, environment sentinel, and unavailable are
// separate states. Copying a value retains actual bytes through frame snapshots.
class MaterialTextureReferenceCandidate {
 std::shared_ptr<const MaterialTextureResourceCandidate>owned;
 bool environmentFlag=false;
public:
 MaterialTextureReferenceCandidate()=default;
 static MaterialTextureReferenceCandidate ordinary(std::shared_ptr<const MaterialTextureResourceCandidate>p)noexcept{MaterialTextureReferenceCandidate r;r.owned=std::move(p);return r;}
 static MaterialTextureReferenceCandidate environment()noexcept{MaterialTextureReferenceCandidate r;r.environmentFlag=true;return r;}
 bool valid()const noexcept{return environmentFlag||bool(owned);}
 bool isEnvironment()const noexcept{return environmentFlag;}
 const std::shared_ptr<const MaterialTextureResourceCandidate>&resource()const noexcept{return owned;}
 friend bool operator==(const MaterialTextureReferenceCandidate&a,const MaterialTextureReferenceCandidate&b)noexcept{return a.environmentFlag==b.environmentFlag&&a.owned==b.owned;}
};
using MaterialTextureResolverCandidate=std::function<std::optional<MaterialTextureReferenceCandidate>(const std::optional<std::string>&)>;
