#include "model_render_inputs_candidate.h"
#include <iostream>
int main(){try{
 auto defaults=materialColorSpaceCandidate(parseVmtCandidate("VertexLitGeneric {}"));if(!defaults.baseSrgb||!defaults.outputSrgb)throw std::runtime_error("color defaults");
 for(int read:{-1,0,1,2})for(int write:{-1,0,1,2}){
  auto syntax=parseVmtCandidate("VertexLitGeneric {$gammacolorread "+std::to_string(read)+" $linearwrite "+std::to_string(write)+"}");
  auto policy=materialColorSpaceCandidate(syntax);if(policy.baseSrgb!=(read!=1)||policy.outputSrgb!=(write!=1))throw std::runtime_error("integer1 policy mismatch");
 }
 unsigned rejected=0;for(const char*text:{"$linearwrite 1 $linearwrite 0","$gammacolorread 1 $GAMMACOLORREAD 0","$linearwrite 1.0","$gammacolorread true","$linearwrite 2147483648","$linearwrite \"\""})try{materialColorSpaceCandidate(parseVmtCandidate(std::string("VertexLitGeneric {")+text+"}"));}catch(const std::runtime_error&){++rejected;}
 if(rejected!=6)throw std::runtime_error("malformed color policy accepted");
 std::filesystem::path mount=R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod)";auto base=mount/"models/anno/mamagen/mamagen";
 auto model=readSfmModelGeometryCandidate(readBinaryAssetCandidate(base.string()+".mdl"),readBinaryAssetCandidate(base.string()+".vvd"),readBinaryAssetCandidate(base.string()+".dx90.vtx"));
 auto input=prepareModelRenderInputsCandidate(model,0,{mount},2,{},[](const auto&,VmtNodeCandidate&syntax){for(const char*key:{"$gammacolorread","$linearwrite"}){VmtNodeCandidate node;node.key=key;node.value="1";syntax.children.push_back(std::move(node));}});
 if(input.materials.empty())throw std::runtime_error("model material coverage");for(const auto&material:input.materials)if(material.colorSpace.baseSrgb||material.colorSpace.outputSrgb)throw std::runtime_error("material color settings lost during move");
 std::cout<<"PASS material color policy:16integer combinations, six malformed rejections, installed model prepared settings preserved; GPU policy selection pending\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
