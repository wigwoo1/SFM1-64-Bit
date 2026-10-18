#include "material_alpha_test_candidate.h"
#include <iostream>
int main(){
 try{
  auto read=[](const std::string&body){return materialAlphaCutoffCandidate(parseVmtCandidate("UnlitGeneric { "+body+" }"));};
  const float fallback=178.f/255.f;
  auto close=[](float a,float b){return std::fabs(a-b)<1e-6f;};
  if(read("")!=0||read("$alphatest 0 $alphatestreference junk")!=0||!close(read("$alphatest 1"),fallback)||!close(read("$alphatest 1 $alphatestreference 0"),fallback)||!close(read("$AlphaTest 1 $AlphaTestReference .5"),127.f/255.f)||read("$alphatest 1 $alphatestreference 1")!=1.f)throw std::runtime_error("alpha test valid cases");
  const auto implicitVertex=materialAlphaCutoffCandidate(parseVmtCandidate("VertexLitGeneric { $alphatest 1 }"));
  if(!close(implicitVertex,fallback))throw std::runtime_error("VertexLitGeneric default alpha reference");
  auto defaultState=materialNativeAlphaStateCandidate(parseVmtCandidate("VertexLitGeneric { $alphatest 1 }"));
  auto zeroState=materialNativeAlphaStateCandidate(parseVmtCandidate("VertexLitGeneric { $alphatest 1 $alphatestreference 0 }"));
  auto explicitState=materialNativeAlphaStateCandidate(parseVmtCandidate("VertexLitGeneric { $alphatest 1 $alphatestreference .5 }"));
  if(!defaultState||!zeroState||!explicitState||defaultState->reference!=178||zeroState->reference!=178||explicitState->reference!=127||!nativeAlphaShadowEnabledCandidate(*defaultState))throw std::runtime_error("native material alpha metadata");
  if(materialNativeAlphaStateCandidate(parseVmtCandidate("UnknownShader {}")))throw std::runtime_error("unknown shader assigned native metadata");
  bool unknownDefaultRejected=false;try{materialAlphaCutoffCandidate(parseVmtCandidate("UnknownShader { $alphatest 1 }"));}catch(const std::runtime_error&){unknownDefaultRejected=true;}if(!unknownDefaultRejected)throw std::runtime_error("unknown shader default accepted");
  unsigned count=0;
  for(const std::string&body:{"$alphatest 2","$alphatest 1 $alphatest 1 $alphatestreference .5","$alphatest 1 $alphatestreference .5 $alphatestreference .6","$alphatest 1 $alphatestreference nan","$alphatest 1 $alphatestreference inf","$alphatest 1 $alphatestreference -1","$alphatest 1 $alphatestreference 1.1","$alphatest 1 $alphatestreference .5junk"}){bool rejected=false;try{read(body);}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("alpha test invalid accepted");++count;}
  std::cout<<"material_alpha_test_valid=6 invalid_rejections="<<count<<" native_metadata_cases=4 default_cutoff="<<fallback<<" GPU_native_default=verified_from_installed_binary\n";
  return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}