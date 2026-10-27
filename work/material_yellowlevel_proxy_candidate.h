#pragma once
#include "material_multiply_proxy_candidate.h"
// Whole original YellowLevel OnBind unconditionally calls SetVecValue(1,1,1).
// It does not inspect bind data, a game entity, time, or the parsed component.
class MaterialYellowLevelProxyCandidate:public MaterialProxyInstanceCandidate {
 MaterialProxyBindingCandidate result;
public:
 bool initialize(const MaterialProxyContextCandidate&context,const SourceMaterialPatchNodeCandidate&declaration)override{
  const auto*field=sourceMaterialPatchFindConstCandidate(declaration,"resultVar");if(!field||field->kind!=1)return false;
  auto name=field->text.substr(0,field->text.find('\0'));if(name.size()>255)return false;
  const auto bracket=name.find('[');if(bracket!=std::string::npos)name.resize(bracket);
  // Basic Init has renderer flag/snapshot side effects for alpha results.
  // Those remain unsupported. Component suffixes otherwise do NOT constrain
  // this proxy: original OnBind always writes the complete three-vector.
  if(sourceMaterialPatchLowerCandidate(name)=="$alpha")return false;
  result=context.find(name);return result.valid();
 }
 bool evaluate(MaterialProxyEvaluationCandidate&frame)override{return frame.setVector3(result,{0x3f800000,0x3f800000,0x3f800000});}
};
// Separate opt-in; existing measured factories and renderer gates unchanged.
struct MaterialYellowLevelProxyFactoryCandidate:MaterialMultiplyPuppetProxyFactoryCandidate {
 MaterialProxyInstanceCandidate*create(const std::string&name)override{return name=="YellowLevel"?new MaterialYellowLevelProxyCandidate:MaterialMultiplyPuppetProxyFactoryCandidate::create(name);}
};
