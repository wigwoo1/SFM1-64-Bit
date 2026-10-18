#include "model_render_inputs_candidate.h"
#include <iostream>
int main(int argc,char**argv){try{
 auto defaults=materialSelfIllumCandidate(parseVmtCandidate("VertexLitGeneric {}"));if(defaults.enabled||defaults.tint!=std::array<float,3>{1,1,1}||!defaults.mask.empty())throw std::runtime_error("selfillum defaults");
 auto hdr=materialSelfIllumCandidate(parseVmtCandidate("VertexLitGeneric { $selfillum 1 $selfillumtint \"[5 5 5]\" $selfillummask \"models\\test\" }"));if(!hdr.enabled||hdr.tint!=std::array<float,3>{5,5,5}||hdr.mask!="models\\test")throw std::runtime_error("authored HDR tint/mask lost");
 unsigned rejected=0;for(auto text:{"$selfillum 2","$selfillum 1 $selfillum 0","$selfillumtint \"[-1 0 0]\"","$selfillumtint \"[1 2]\"","$selfillumtint \"[1 2 3 4]\"","$selfillummask a $selfillummask b"}){try{materialSelfIllumCandidate(parseVmtCandidate(std::string("VertexLitGeneric { ")+text+" }"));}catch(const std::runtime_error&){++rejected;}}
 if(rejected!=6)throw std::runtime_error("malformed selfillum accepted");
 if(argc!=3)throw std::runtime_error("model base and mount required");std::filesystem::path base=argv[1];auto model=readSfmModelGeometryCandidate(readBinaryAssetCandidate(base.string()+".mdl"),readBinaryAssetCandidate(base.string()+".vvd"),readBinaryAssetCandidate(base.string()+".dx90.vtx"));
 auto inputs=prepareModelRenderInputsCandidate(model,0,{argv[2]},2);bool visor=false;
 for(const auto&material:inputs.materials)if(vmtAsciiLowerCandidate(material.source.vmt.stem().string())=="visor"){
  visor=true;if(!material.selfIllum.enabled||material.selfIllum.tint!=std::array<float,3>{5,5,5}||material.source.selfIllumMaskTexture.stem()!="visorglow"||material.selfIllumMaskImage.rgba.empty())throw std::runtime_error("installed visor selfillum data missing");
  auto bytes=readBinaryAssetCandidate(material.source.selfIllumMaskTexture);auto layout=readVtfTextureLayoutCandidate(bytes);auto expected=decodeVtfMipCandidate(bytes,std::min(uint32_t(2),uint32_t(layout.mips.size()-1)),0);
  if(expected.width!=material.selfIllumMaskImage.width||expected.height!=material.selfIllumMaskImage.height||expected.rgba!=material.selfIllumMaskImage.rgba)throw std::runtime_error("decoded mask differs");
  std::cout<<"visor_mask="<<expected.width<<'x'<<expected.height<<" hdr_tint=5,5,5\n";
 }
 if(!visor)throw std::runtime_error("visor fixture not exercised");
 std::cout<<"PASS selfillum defaults, HDR tint, six malformed rejections and installed model mask preparation; GPU composition not implemented\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
