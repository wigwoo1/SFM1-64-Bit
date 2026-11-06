#pragma once
#include "sfm_model_geometry.h"
#include "read_binary_asset_candidate.h"
#include "mounted_model_material_candidate.h"
#include "vtf_block_decode.h"
#include "model_prepared_texture_candidate.h"
#include "material_blend_candidate.h"
#include "material_alpha_test_candidate.h"
#include "material_constant_color_candidate.h"
#include "material_color_space_candidate.h"
#include "material_uv_transform_candidate.h"
#include <sstream>
#include <locale>
#include <map>
#include <limits>
#include <cstdlib>
#include <optional>
// c10.z in the supported Phong shader variants. Zero selects the texture red
// channel; a missing optional means the authored scalar could not be resolved.
inline std::optional<float> phongExponentControlCandidate(const VmtNodeCandidate& syntax){
 const auto values=vmtValuesCandidate(syntax,"$phongexponent");
 if(values.empty())return 0.f;
 if(values.size()!=1)return {};
 std::istringstream input(values.front());
 input.imbue(std::locale::classic());
 float value;
 if(!(input>>value) || !std::isfinite(value))return {};
 input>>std::ws;
 if(!input.eof())return {};
 return value;
}
// Experimental HDR2 uses the confirmed r_rimlight=1 benchmark profile.
inline std::array<float,2> unmaskedRimControlsCandidate(const VmtNodeCandidate& syntax){
 auto scalar=[&](const char* key,float fallback)->std::optional<float>{
  auto values=vmtValuesCandidate(syntax,key);
  if(values.empty())return fallback;
  if(values.size()!=1)return {};
  std::istringstream in(values.front());in.imbue(std::locale::classic());
  float value;if(!(in>>value)||!std::isfinite(value))return {};
  in>>std::ws;if(!in.eof())return {};
  return value;
 };
 auto enabled=scalar("$rimlight",0),mask=scalar("$rimmask",0);
 if(!enabled || *enabled==0 || !mask || *mask!=0)return {};
 auto exponent=scalar("$rimlightexponent",4),boost=scalar("$rimlightboost",1);
 if(!exponent || !boost)return {};
 return {std::max(1.f,*exponent),*boost};
}
struct ScalarSkyPhongCandidate {
 std::array<float,2> rim{};
 std::array<float,4> fresnelBoost{};
 bool useBaseAlpha=false;
};
inline std::optional<ScalarSkyPhongCandidate> scalarSkyPhongCandidate(const VmtNodeCandidate& syntax){
 const auto is=[&](const char* key,const char* value){return vmtValuesCandidate(syntax,key)==std::vector<std::string>{value};};
 if(!is("$phong","1"))return {};
 const auto mask=vmtValuesCandidate(syntax,"$basemapalphaphongmask");
 if(!mask.empty() && !is("$basemapalphaphongmask","0") && !is("$basemapalphaphongmask","1"))return {};
 for(auto key:{"$basemapluminancephongmask","$invertphongmask","$phongalbedotint"})
  if(!vmtValuesCandidate(syntax,key).empty()&&!is(key,"0"))return {};
 if(!vmtValuesCandidate(syntax,"$phongwarptexture").empty())return {};
 if(!vmtValuesCandidate(syntax,"$phongtint").empty()&&!is("$phongtint","[0 0 0]"))return {};
 auto f=vmtValuesCandidate(syntax,"$phongfresnelranges"),b=vmtValuesCandidate(syntax,"$phongboost");
 if(f.size()!=1 || b.size()!=1 || f[0].size()<2 || f[0].front()!='[' || f[0].back()!=']')return {};
 auto boostText=b[0];
 if(boostText.size()>1 && boostText.front()=='[' && boostText.back()==']')boostText=boostText.substr(1,boostText.size()-2);
 std::istringstream ranges(f[0].substr(1,f[0].size()-2)),boost(boostText);
 ranges.imbue(std::locale::classic());boost.imbue(std::locale::classic());
 std::array<float,4> out{};
 if(!(ranges>>out[0]>>out[1]>>out[2]) || !(boost>>out[3]))return {};
 ranges>>std::ws;boost>>std::ws;
 if(!ranges.eof()||!boost.eof())return {};
 for(float value:out)if(!std::isfinite(value)||value<0)return {};
 return ScalarSkyPhongCandidate{unmaskedRimControlsCandidate(syntax),out,is("$basemapalphaphongmask","1")};
}
struct ModelLightwarpInputsCandidate {
 std::optional<ScalarSkyPhongCandidate> scalarSkyPhong;
 std::optional<float> phongExponentControl;
 ModelPreparedTextureCandidate warp,normal,exponent;
};
struct ModelRenderMaterialCandidate {MountedModelMaterialCandidate source;VtfRgbaImageCandidate image;bool translucent=false;VtfRgbaImageCandidate irisImage;float alphaCutoff=0;std::optional<NativeAlphaShadowStateCandidate>nativeAlpha;MaterialSelfIllumCandidate selfIllum;VtfRgbaImageCandidate selfIllumMaskImage;MaterialColorSpaceCandidate colorSpace;MaterialUvTransformCandidate baseTextureTransform=identityMaterialUvTransformCandidate();bool vertexLighting=false;VtfRgbaImageCandidate secondBaseImage;bool vertexTransition=false;bool unlitTwoTexture=false,additive=false;MaterialUvTransformCandidate secondTextureTransform=identityMaterialUvTransformCandidate();std::optional<uint32_t>baseTextureFlags,secondTextureFlags;ModelPreparedTextureCandidate preparedBase,preparedSecond;ModelLightwarpInputsCandidate lightwarpInputs;ModelPreparedTextureCandidate selfShadowedBump;};
inline VtfRgbaImageCandidate missingSourceTextureFallbackCandidate(){return {2,2,{255,0,255,255,0,0,0,255,0,0,0,255,255,0,255,255}};}
struct ModelRenderRangeCandidate {uint32_t firstIndex,indexCount,materialIndex,body,model,mesh;int32_t eyeball=-1;};
struct ModelRenderInputsCandidate {std::vector<uint32_t>indices;std::vector<ModelRenderRangeCandidate>ranges;std::vector<ModelRenderMaterialCandidate>materials;};
inline ModelRenderInputsCandidate prepareModelRenderInputsCandidate(const SfmModelGeometryCandidate&model,size_t skin,const std::vector<std::filesystem::path>&mounts,uint32_t mip,const std::vector<uint32_t>&bodySelection={},const std::function<void(const std::filesystem::path&,VmtNodeCandidate&)>&applyOverrides={},uint32_t textureFrame=0){
 if(!bodySelection.empty()&&bodySelection.size()!=model.bodyModelCounts.size())throw std::runtime_error("body selection count");
 for(size_t n=0;n<model.bodyModelCounts.size();++n)if((bodySelection.empty()?0:bodySelection[n])>=model.bodyModelCounts[n])throw std::runtime_error("body model selection");
 ModelRenderInputsCandidate result;std::map<std::string,uint32_t>loaded;
 for(const auto&mesh:model.meshes){if(mesh.body>=model.bodyModelCounts.size()||mesh.model>=model.bodyModelCounts[mesh.body])throw std::runtime_error("mesh body/model identity");if(mesh.model!=(bodySelection.empty()?0:bodySelection[mesh.body]))continue;const auto&name=model.materials.resolve(skin,mesh.material);auto found=loaded.find(name);uint32_t material;
  if(found==loaded.end()){auto source=resolveMountedModelMaterialCandidate(model.materials,skin,mesh.material,mounts,applyOverrides);auto uvTransform=materialUvTransformCandidate(source.syntax);bool unlitTwoTexture=vmtAsciiLowerCandidate(source.syntax.key)=="unlittwotexture";auto secondTransform=unlitTwoTexture?materialUvTransformCandidate(source.syntax,"$texture2transform"):identityMaterialUvTransformCandidate();bool additive=unlitTwoTexture&&materialAdditiveCandidate(source.syntax);std::optional<uint32_t>baseTextureFlags,secondTextureFlags;ModelPreparedTextureCandidate preparedBase,preparedSecond;VtfRgbaImageCandidate image;if(source.baseTextureMissing){image=missingSourceTextureFallbackCandidate();}else if(source.baseTexture.empty()){auto color=materialConstantColorCandidate(source.syntax);image={1,1,{color.begin(),color.end()}};}else{auto bytes=readBinaryAssetCandidate(source.baseTexture);VtfTextureLayoutCandidate textureLayout;try{textureLayout=readVtfTextureLayoutCandidate(bytes);}catch(const std::exception&e){throw std::runtime_error("base VTF unsupported for "+source.baseTexture.string()+": "+e.what());}auto selectedMip=mip<uint32_t(textureLayout.mips.size())?mip:uint32_t(textureLayout.mips.size()-1);baseTextureFlags=textureLayout.flags;image=decodeVtfMipCandidate(bytes,selectedMip,textureLayout.frameCount==1?0:textureFrame);preparedBase=prepareModelTextureSnapshotCandidate(vmtValuesCandidate(source.syntax,"$basetexture").at(0),bytes,mip,textureLayout.frameCount==1?0:textureFrame);}VtfRgbaImageCandidate secondBaseImage;if(!source.baseTexture2.empty()){auto bytes=readBinaryAssetCandidate(source.baseTexture2);VtfTextureLayoutCandidate textureLayout;try{textureLayout=readVtfTextureLayoutCandidate(bytes);}catch(const std::exception&e){throw std::runtime_error("second base VTF unsupported for "+source.baseTexture2.string()+": "+e.what());}auto selectedMip=mip<uint32_t(textureLayout.mips.size())?mip:uint32_t(textureLayout.mips.size()-1);secondTextureFlags=textureLayout.flags;secondBaseImage=decodeVtfMipCandidate(bytes,selectedMip,textureLayout.frameCount==1?0:textureFrame);preparedSecond=prepareModelTextureSnapshotCandidate(vmtValuesCandidate(source.syntax,unlitTwoTexture?"$texture2":"$basetexture2").at(0),bytes,mip,textureLayout.frameCount==1?0:textureFrame);}if(result.materials.size()>=UINT32_MAX)throw std::runtime_error("material count overflow");material=uint32_t(result.materials.size());auto translucent=materialTranslucentCandidate(source.syntax);auto alphaCutoff=materialAlphaCutoffCandidate(source.syntax);auto nativeAlpha=materialNativeAlphaStateCandidate(source.syntax);VtfRgbaImageCandidate irisImage;if(!source.irisTexture.empty()){auto irisBytes=readBinaryAssetCandidate(source.irisTexture);auto irisLayout=readVtfTextureLayoutCandidate(irisBytes);auto irisMip=mip<uint32_t(irisLayout.mips.size())?mip:uint32_t(irisLayout.mips.size()-1);irisImage=decodeVtfMipCandidate(irisBytes,irisMip,irisLayout.frameCount==1?0:textureFrame);}auto selfIllum=materialSelfIllumCandidate(source.syntax);if(source.selfIllumMaskMissing)selfIllum.mask.clear();VtfRgbaImageCandidate selfIllumMaskImage;if(!source.selfIllumMaskTexture.empty()){auto maskBytes=readBinaryAssetCandidate(source.selfIllumMaskTexture);auto maskLayout=readVtfTextureLayoutCandidate(maskBytes);auto maskMip=mip<uint32_t(maskLayout.mips.size())?mip:uint32_t(maskLayout.mips.size()-1);selfIllumMaskImage=decodeVtfMipCandidate(maskBytes,maskMip,maskLayout.frameCount==1?0:textureFrame);}auto colorSpace=materialColorSpaceCandidate(source.syntax);bool vertexTransition=vmtAsciiLowerCandidate(source.syntax.key)=="worldvertextransition";result.materials.push_back({std::move(source),std::move(image),translucent,std::move(irisImage),alphaCutoff,nativeAlpha,std::move(selfIllum),std::move(selfIllumMaskImage),colorSpace,uvTransform,false,std::move(secondBaseImage),vertexTransition,unlitTwoTexture,additive,secondTransform,baseTextureFlags,secondTextureFlags,std::move(preparedBase),std::move(preparedSecond)});loaded.emplace(name,material);}else material=found->second;
  uint64_t count=uint64_t(mesh.triangles.size())*3;if(count>UINT32_MAX||result.indices.size()>UINT32_MAX-count)throw std::runtime_error("render index count overflow");int32_t eyeball=-1;if(mesh.materialType==1){if(mesh.materialParameter<0)throw std::runtime_error("negative mesh eyeball index");int32_t localEye=0;for(size_t eye=0;eye<model.eyeballs.size();++eye){auto&record=model.eyeballs[eye];if(record.body!=mesh.body||record.model!=mesh.model)continue;if(localEye++==mesh.materialParameter){if(eye>INT32_MAX)throw std::runtime_error("eyeball index overflow");eyeball=int32_t(eye);break;}}if(eyeball<0)throw std::runtime_error("mesh eyeball record missing");}result.ranges.push_back({uint32_t(result.indices.size()),uint32_t(count),material,mesh.body,mesh.model,mesh.mesh,eyeball});for(const auto&triangle:mesh.triangles)for(auto vertex:triangle){if(vertex>=model.geometry.vertices.size())throw std::runtime_error("render vertex index");result.indices.push_back(vertex);}
 }
 // Isolated diagnostic fixture changes owned decoded inputs only, never assets.
 if(const char*fixture=std::getenv("SFM64_DIAGNOSTIC_SELFILLUM_FIXTURE")){
  std::string mode(fixture);if(mode!="black"&&mode!="red"&&mode!="white"&&mode!="alpha"&&mode!="alpha-half"&&mode!="red-half")throw std::runtime_error("selfillum fixture mode");
  const char*enabled=std::getenv("SFM64_DIAGNOSTIC_SELFILLUM_TEST");if(!enabled||std::string(enabled)!="1")throw std::runtime_error("selfillum fixture requires candidate shader");
  for(auto&material:result.materials){material.preparedBase={};material.image={1,1,{64,128,192,255}};material.translucent=false;material.alphaCutoff=0;material.selfIllum={true,{.5f,1.f,5.f},"diagnostic-owned-mask"};material.selfIllumMaskImage={1,1,{uint8_t(mode=="black"?0:255),uint8_t(mode=="white"?255:0),uint8_t(mode=="white"?255:0),255}};if(mode=="red-half")material.selfIllumMaskImage.rgba[0]=128;if(mode=="alpha"||mode=="alpha-half"){material.selfIllum.mask.clear();material.selfIllumMaskImage={};if(mode=="alpha-half")material.image.rgba[3]=128;}}
  if(const char*readMode=std::getenv("SFM64_DIAGNOSTIC_FIXTURE_GAMMA_READ")){
   std::string read(readMode);if(read!="0"&&read!="1"&&read!="mixed")throw std::runtime_error("fixture gamma-read mode");
   for(size_t n=0;n<result.materials.size();++n){auto&material=result.materials[n];auto&syntax=material.source.syntax;syntax.children.erase(std::remove_if(syntax.children.begin(),syntax.children.end(),[](const auto&node){return vmtAsciiLowerCandidate(node.key)=="$gammacolorread";}),syntax.children.end());VmtNodeCandidate parameter;parameter.key="$gammacolorread";parameter.value=read=="mixed"?(n%2?"1":"0"):read;syntax.children.push_back(std::move(parameter));material.colorSpace=materialColorSpaceCandidate(syntax);}
  }
 }
 return result;
}
