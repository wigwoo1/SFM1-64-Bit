#pragma once
#include "material_parameter_backup_candidate.h"

// Value-only adapter for the already measured typed dispatch. Texture handles
// are not represented/owned here; type5 must not be reinterpreted as SetString.
class MaterialParameterOverrideServicesCandidate:public MaterialSessionOverrideServicesCandidate {
 MaterialParameterCollectionCandidate&parameters;
 MaterialParameterBackupServicesCandidate names;
 MaterialTypedDispatchCandidate&at(uintptr_t token){
  if(!token||token>parameters.values.size())throw std::runtime_error("material override token");return parameters.values[token-1];
 }
 void write(uintptr_t token,uint32_t operation,const std::array<uint32_t,4>&args,uint32_t count=0){
  if(!materialTypedNumericWriteCandidate(at(token),operation,args,count))throw std::runtime_error("material override value unavailable");
 }
public:
 explicit MaterialParameterOverrideServicesCandidate(MaterialParameterCollectionCandidate&values):parameters(values),names(values){}
 MaterialSessionOverrideLookupCandidate find(std::string_view name)override{return names.find(name);}
 void integer(uintptr_t p,uint32_t v)override{write(p,0,{v,0,0,0});}
 void scalar(uintptr_t p,uint32_t v)override{write(p,1,{v,0,0,0});}
 void vector(uintptr_t p,const std::array<uint32_t,4>&v,uint32_t n)override{write(p,5,v,n);}
 void matrix(uintptr_t p,const std::array<uint32_t,16>&v)override{if(!materialTypedMatrixSetCandidate(at(p),v))throw std::runtime_error("material matrix override unavailable");}
 std::optional<uintptr_t>texture(const std::optional<std::string>&)override{return {};}
 void setTexture(uintptr_t,uintptr_t)override{throw std::runtime_error("material texture ownership unavailable");}
};
inline bool applyMaterialParameterOverridesCandidate(MaterialParameterCollectionCandidate&parameters,const std::vector<MaterialSessionOverrideInputCandidate>&rows){
 try{auto plan=parameters;MaterialParameterOverrideServicesCandidate service(plan);if(!dispatchMaterialSessionOverridesCandidate(rows,service))return false;parameters=std::move(plan);return true;}catch(...){return false;}
}
