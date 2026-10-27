#pragma once
#include "material_weapon_invis_puppet_proxy_candidate.h"
#include <xmmintrin.h>
extern "C" uint32_t materialMultiplyScalarX87Candidate(uint32_t first,uint32_t second);
struct MaterialMultiplyResultCandidate {
 bool write=false;uint32_t operation=0,count=0;std::array<uint32_t,4>arguments{};
};
// Original effective result type controls arithmetic, not source spelling.
// Only the bounded CPU/value domain is represented, with default FP modes.
inline std::optional<MaterialMultiplyResultCandidate>materialMultiplyResultCandidate(
 const MaterialNumericValueCandidate&output,const MaterialNumericValueCandidate&first,
 const MaterialNumericValueCandidate&second,int32_t component){
 MaterialMultiplyResultCandidate result;auto type=output.typeAndMetadata&15u;auto count=(output.typeAndMetadata>>4)&7u;
 if(type==2&&component>=0)type=0;
 else if(type==6){type=first.typeAndMetadata&15u;count=(first.typeAndMetadata>>4)&7u;if(type==6){type=second.typeAndMetadata&15u;count=(second.typeAndMetadata>>4)&7u;}}
 if(type!=0&&type!=2&&type!=4)return result;
 result.write=true;
 if(type==4){result.operation=1;result.arguments[0]=materialNumericIntFloatCandidate(first.integer*second.integer);return result;}
 auto product=materialMultiplyScalarX87Candidate;
 if(type==2){
  if(count<1||count>4)return {};result.operation=5;result.count=count;
  for(uint32_t i=0;i<std::min(count,3u);++i){auto left=second.vector[i],right=first.vector[i];
   // MULSS destination is the SECOND source. Preserve its NaN precedence,
   // independently of compiler commutation/register allocation choices.
   if((left&0x7fffffffu)>0x7f800000u)result.arguments[i]=left|0x400000u;
   else if((right&0x7fffffffu)>0x7f800000u)result.arguments[i]=right|0x400000u;
   else{float a,b;std::memcpy(&a,&left,4);std::memcpy(&b,&right,4);float value=_mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));std::memcpy(&result.arguments[i],&value,4);}
  }
  // Whole native OnBind computes only three products; its fourth outgoing
  // stack word aliases the first input's first component. This is deliberate
  // observed compatibility behavior, not a fourth multiplication.
  if(count==4)result.arguments[3]=first.vector[0];return result;
 }
 const auto bits=product(first.vector[0],second.vector[0]);
 if((output.typeAndMetadata&15u)==2){
  const auto n=(output.typeAndMetadata>>4)&7u;if(n>4||component>3)return {};
  if(component>=0){result.operation=8;result.count=uint32_t(component);result.arguments[0]=bits;}
  else{if(n<1)return {};result.operation=5;result.count=n;result.arguments.fill(bits);for(uint32_t i=n;i<4;++i)result.arguments[i]=0;}
 }else{result.operation=1;result.arguments[0]=bits;}
 return result;
}
class MaterialMultiplyProxyCandidate:public MaterialProxyInstanceCandidate {
 MaterialProxyBindingCandidate result,first,second;int32_t component=-1;
public:
 bool initialize(const MaterialProxyContextCandidate&context,const SourceMaterialPatchNodeCandidate&declaration)override{
  auto field=[&](const char*key,std::string&text){auto*p=sourceMaterialPatchFindConstCandidate(declaration,key);if(!p){text.clear();return true;}if(p->kind!=1||p->text.size()>255)return false;text=p->text;return true;};
  std::string output,a,b;if(!field("resultVar",output)||!field("srcVar1",a)||!field("srcVar2",b)||b.empty())return false;
  component=-1;auto bracket=output.find('[');if(bracket!=std::string::npos){component=int32_t(materialStringIntegerCandidate(output.substr(bracket+1)));output.resize(bracket);if(component>3)return false;}
  if(sourceMaterialPatchLowerCandidate(output)=="$alpha")return false; // Renderer flag/snapshot side effects remain outside CPU gate.
  result=context.find(output);first=context.find(a);second=context.find(b);return result.valid()&&first.valid()&&second.valid();
 }
 bool evaluate(MaterialProxyEvaluationCandidate&frame)override{
  auto out=frame.read(result),a=frame.read(first),b=frame.read(second);if(!out||!a||!b)return false;
  auto plan=materialMultiplyResultCandidate(out->value.numeric,a->value.numeric,b->value.numeric,component);if(!plan)return false;if(!plan->write)return true;
  if(plan->operation==5)return frame.setVector(result,plan->arguments,plan->count);
  if(plan->operation==8)return frame.setComponent(result,plan->count,plan->arguments[0]);return frame.setFloat(result,plan->arguments[0]);
 }
};
// Separate opt-in until whole shaders/clock/renderer resources are supported.
// Existing factories and their honest unsupported-proxy behavior stay intact.
struct MaterialMultiplyPuppetProxyFactoryCandidate:MaterialPuppetFunctionProxyFactoryCandidate {
 MaterialProxyInstanceCandidate*create(const std::string&name)override{return name=="Multiply"?new MaterialMultiplyProxyCandidate:MaterialPuppetFunctionProxyFactoryCandidate::create(name);}
};
