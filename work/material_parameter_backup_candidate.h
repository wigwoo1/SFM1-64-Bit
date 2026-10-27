#pragma once
#include "material_parameter_collection_candidate.h"
#include "material_matrix_value_write_candidate.h"
#include "material_get_string_candidate.h"
#include "material_session_backup_candidate.h"
#include <functional>

// An explicit, already-retained resource's name projection, NOT a resource owner
// or pointer. Only the caller can establish which texture/material it denotes.
struct MaterialParameterResourceTextCandidate {
 std::optional<std::string>name;
 bool environmentTexture=false;
};
using MaterialParameterResourceTextServiceCandidate=std::function<MaterialParameterResourceTextCandidate(size_t,const MaterialTypedDispatchCandidate&)>;

// Convert the EXISTING typed representation to a GetString owned snapshot.
// Numeric caches remain numeric; optional matrix storage is never stringified
// in the material state. Unknown storage kinds have no invented empty fallback.
inline bool materialParameterStringSnapshotCandidate(const MaterialTypedDispatchCandidate&value,
        std::string&output,const MaterialParameterResourceTextCandidate&resource={}){
 if(!value.created)return false;
 MaterialGetStringInputCandidate input;const auto&numeric=value.value.numeric;
 input.type=numeric.typeAndMetadata&15u;input.components=(numeric.typeAndMetadata>>4)&7u;
 input.integer=numeric.integer;std::copy(numeric.vector.begin(),numeric.vector.end(),input.numbers.begin());
 switch(input.type){
 case 0:case 2:case 4:case 6:break;
 case 1:input.text=value.value.text;break;
 case 7:if(!value.matrix)return false;input.numbers=*value.matrix;break;
 case 3:case 8:input.resourceName=resource.name;input.environmentTexture=input.type==3&&resource.environmentTexture;break;
 default:return false;
 }
 return materialGetStringCandidate(input,output);
}

// Owner-thread snapshot of retained CPU parameters. No pointer into the store
// survives construction; a resource service may reenter/retire its source store.
// Native queue/undo/notification ownership is outside this value adapter.
class MaterialParameterBackupServicesCandidate:public MaterialSessionBackupServicesCandidate {
 MaterialParameterCollectionCandidate parameters;
 MaterialParameterResourceTextServiceCandidate resourceText;
 const MaterialTypedDispatchCandidate&at(uintptr_t token)const{
  if(!token||token>parameters.values.size())throw std::runtime_error("material backup variable token");
  const auto&value=parameters.values[token-1];if(!value.created)throw std::runtime_error("material backup variable unavailable");return value;
 }
 const MaterialNumericValueCandidate&cached(uintptr_t token)const{
  const auto&value=at(token);const auto type=value.value.numeric.typeAndMetadata&15u;
  if(!materialStringDomainCandidate(type)&&(type!=7||!value.matrix))throw std::runtime_error("material backup numeric cache unavailable");
  return value.value.numeric;
 }
 static std::string key(std::string_view s){s=s.substr(0,s.find('\0'));std::string result(s);for(char&c:result)if(c>='A'&&c<='Z')c=char(c+32);return result;}
public:
 explicit MaterialParameterBackupServicesCandidate(const MaterialParameterCollectionCandidate&source,
         MaterialParameterResourceTextServiceCandidate resources={}):parameters(source),resourceText(std::move(resources)){
  if(parameters.values.size()>256)throw std::runtime_error("material backup parameter extent");
  for(const auto&v:parameters.values)if(v.name.size()>1023)throw std::runtime_error("material backup name extent");
 }
 MaterialSessionOverrideLookupCandidate find(std::string_view name)override{
  const auto wanted=key(name);for(size_t i=parameters.values.size();i>0;--i)if(parameters.values[i-1].created&&key(parameters.values[i-1].name)==wanted)return {i,true};return {};
 }
 uint32_t integer(uintptr_t p)override{return cached(p).integer;}
 // Original x87 scalar/vector loads quiet signaling NaNs in the returned copy,
 // without rewriting the retained cached bits. Matrix pointers do not do so.
 uint32_t scalar(uintptr_t p)override{return materialSessionBackupQuietCandidate(cached(p).vector[0]);}
 std::array<uint32_t,4>vector(uintptr_t p,uint32_t n)override{
  if(n<2||n>4)throw std::runtime_error("material backup vector extent");
  const auto&input=cached(p).vector;std::array<uint32_t,4>result{};
  // Read the requested count, not the stored component count. Unrequested array
  // slots are owned padding zeroes; the original writes only n caller slots.
  for(unsigned i=0;i<n;++i)result[i]=materialSessionBackupQuietCandidate(input[i]);return result;
 }
 std::array<uint32_t,16>matrix(uintptr_t p)override{
  const auto result=materialTypedMatrixReadCandidate(at(p));if(!result)throw std::runtime_error("material backup matrix unavailable");return *result;
 }
 std::optional<std::string>stringValue(uintptr_t p)override{
  const auto&value=at(p);MaterialParameterResourceTextCandidate resource;
  const auto type=value.value.numeric.typeAndMetadata&15u;
  if((type==3||type==8)&&resourceText)resource=resourceText(size_t(p-1),value);
  std::string result;if(!materialParameterStringSnapshotCandidate(value,result,resource))return {};return result;
 }
};
inline bool captureMaterialParameterBackupCandidate(std::vector<MaterialSessionOverrideInputCandidate>&rows,
        const MaterialParameterCollectionCandidate&source,int phase,
        const MaterialParameterResourceTextServiceCandidate&resources={}){
 try{MaterialParameterBackupServicesCandidate service(source,resources);return captureMaterialSessionBackupCandidate(rows,service,phase);}
 catch(...){return false;} // Existing capture stages rows; service failure cannot publish a prefix.
}
