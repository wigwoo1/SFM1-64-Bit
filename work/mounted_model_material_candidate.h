#pragma once
#include "model_material_metadata.h"
#include "vmt_keyvalues_candidate.h"
#include "material_selfillum_candidate.h"
#include <filesystem>
#include <fstream>
#include <functional>
// Loose-file mounts only; caller supplies mounts in priority order.
inline std::filesystem::path materialRelativePathCandidate(std::string name,const char*extension){
 for(char&c:name)if(c=='\\')c='/';
 std::filesystem::path p(name);
 if(name.empty()||p.is_absolute()||p.has_root_directory()||p.has_root_name()||name.find(':')!=std::string::npos)throw std::runtime_error("invalid mounted material path");
 for(const auto&part:p)if(part=="..")throw std::runtime_error("material path traversal");
 p.replace_extension(extension);return p;
}
inline std::filesystem::path findMountedMaterialCandidate(const std::vector<std::filesystem::path>&mounts,const std::filesystem::path&relative){
 for(const auto&mount:mounts){auto path=mount/"materials"/relative;if(std::filesystem::is_regular_file(path))return path;}
 return {};
}
struct MountedModelMaterialCandidate {std::filesystem::path vmt,baseTexture,irisTexture;VmtNodeCandidate syntax;std::filesystem::path selfIllumMaskTexture,baseTexture2;bool baseTextureMissing=false,selfIllumMaskMissing=false,baseTexture2Missing=false;std::vector<std::string>missingTextureReferences;};
inline MountedModelMaterialCandidate resolveMountedModelMaterialCandidate(const ModelMaterialMetadataCandidate&metadata,size_t family,int32_t slot,const std::vector<std::filesystem::path>&mounts,const std::function<void(const std::filesystem::path&,VmtNodeCandidate&)>&applyOverrides={}){
 const auto&name=metadata.resolve(family,slot);MountedModelMaterialCandidate result;
 for(const auto&directory:metadata.directories){result.vmt=findMountedMaterialCandidate(mounts,materialRelativePathCandidate(directory+name,".vmt"));if(!result.vmt.empty())break;}
 if(result.vmt.empty())throw std::runtime_error("model VMT missing: "+name);
 std::ifstream input(result.vmt,std::ios::binary);if(!input)throw std::runtime_error("VMT read failed: "+result.vmt.string());std::string source((std::istreambuf_iterator<char>(input)),{});try{result.syntax=parseVmtCandidate(source);}catch(const std::exception&error){throw std::runtime_error("VMT parse failed: "+result.vmt.string()+": "+error.what());}if(applyOverrides)applyOverrides(result.vmt,result.syntax);
 auto selfIllum=materialSelfIllumCandidate(result.syntax);if(selfIllum.enabled&&!selfIllum.mask.empty()){result.selfIllumMaskTexture=findMountedMaterialCandidate(mounts,materialRelativePathCandidate(selfIllum.mask,".vtf"));if(result.selfIllumMaskTexture.empty()){result.selfIllumMaskMissing=true;result.missingTextureReferences.push_back("$selfillummask "+selfIllum.mask);}}
 auto textures=vmtValuesCandidate(result.syntax,"$basetexture");if(textures.empty()&&vmtAsciiLowerCandidate(result.syntax.key)=="vertexlitgeneric"){if(!vmtValuesCandidate(result.syntax,"$envmap").empty())throw std::runtime_error("untextured envmap shader unfinished: "+result.vmt.string());return result;}if(textures.size()!=1)throw std::runtime_error("exactly one base texture required: "+result.vmt.string()+" shader="+result.syntax.key+" found="+std::to_string(textures.size()));
 result.baseTexture=findMountedMaterialCandidate(mounts,materialRelativePathCandidate(textures[0],".vtf"));if(result.baseTexture.empty()){result.baseTextureMissing=true;result.missingTextureReferences.push_back("$basetexture "+textures[0]);}const bool unlitTwoTexture=vmtAsciiLowerCandidate(result.syntax.key)=="unlittwotexture";const char*secondParameter=unlitTwoTexture?"$texture2":"$basetexture2";auto textures2=vmtValuesCandidate(result.syntax,secondParameter);bool vertexTransition=vmtAsciiLowerCandidate(result.syntax.key)=="worldvertextransition";if(textures2.size()>1)throw std::runtime_error("duplicate second base texture: "+result.vmt.string());if(vertexTransition&&textures2.size()!=1)throw std::runtime_error("WorldVertexTransition requires one $basetexture2: "+result.vmt.string());if(!textures2.empty()){result.baseTexture2=findMountedMaterialCandidate(mounts,materialRelativePathCandidate(textures2[0],".vtf"));if(result.baseTexture2.empty()){result.baseTexture2Missing=true;result.missingTextureReferences.push_back(std::string(secondParameter)+" "+textures2[0]);}}auto iris=vmtValuesCandidate(result.syntax,"$iris");if(iris.size()>1)throw std::runtime_error("duplicate iris texture");if(!iris.empty()){result.irisTexture=findMountedMaterialCandidate(mounts,materialRelativePathCandidate(iris.front(),".vtf"));if(result.irisTexture.empty())throw std::runtime_error("iris texture missing: "+iris.front()+" vmt="+result.vmt.string());}return result;
}
