#pragma once
#include "material_modelglowcolor_proxy_candidate.h"
#include <string_view>
// Explicit pinned CLIENT+IFM static-service profile only. Selecting this type
// asserts that other/dynamic tool providers are outside the requested profile.
// Registry identity evidence: client91dd1062..., IFM8576a42d... (full receipt).
// No engine DLL is required by this owned implementation.
enum class MaterialPinnedProxyClassificationCandidate {Implemented,RegisteredUnimplemented,OriginalFactoryMiss,Unverified};
struct MaterialPinnedClientIfmProxyFactoryCandidate:MaterialModelGlowColorProxyFactoryCandidate {
 inline static constexpr std::string_view clientNames[]={
  "AnimatedEntityTexture",
  "AnimatedOffsetTexture",
  "AnimatedTexture",
  "AnimateSpecificTexture",
  "BreakableSurface",
  "ConveyorScroll",
  "MaterialModify",
  "MaterialModifyAnimated",
  "Camo",
  "Shadow",
  "ShadowModel",
  "ConVar",
  "Cycle",
  "Dummy",
  "EntityOrigin",
  "EntityOriginAlyx",
  "Ep1IntroVortRefract",
  "IsNPC",
  "lampbeam",
  "lamphalo",
  "Add",
  "Subtract",
  "Multiply",
  "Divide",
  "Clamp",
  "Sine",
  "Equals",
  "Frac",
  "Int",
  "LinearRamp",
  "UniformNoise",
  "GaussianNoise",
  "Exponential",
  "Abs",
  "Empty",
  "LessOrEqual",
  "WrapMinMax",
  "SelectFirstIfNonZero",
  "TextureTransform",
  "MatrixRotate",
  "ParticleSphereProxy",
  "Health",
  "PlayerProximity",
  "PlayerTeamMatch",
  "PlayerView",
  "PlayerSpeed",
  "PlayerPosition",
  "EntitySpeed",
  "EntityRandom",
  "PlayerLogo",
  "Pupil",
  "TextureScroll",
  "CurrentTime",
  "ToggleTexture",
  "BloomAdd",
  "engine_post",
  "MotionBlur",
  "WaterLOD",
  "WorldDims",
  "ObjectPower",
  "spy_invis",
  "InvulnLevel",
  "BurnLevel",
  "YellowLevel",
  "ModelGlowColor",
  "CommunityWeapon",
  "HeartbeatScale",
  "BenefactorLevel",
  "ItemTintColor",
  "StickybombGlowColor",
  "TeamTexture",
  "vm_invis",
  "SniperRifleCharge",
  "weapon_invis",
 };
 inline static constexpr std::string_view ifmInterfaces[]={
  "accumbuff4sample_IMaterialProxy003",
  "sfm_downsample_proxy_IMaterialProxy003",
  "sfm_shape_proxy_IMaterialProxy003",
  "sfm_blurfilterx_proxy_IMaterialProxy003",
  "sfm_blurfiltery_proxy_IMaterialProxy003",
  "sfm_integercombine_proxy_IMaterialProxy003",
  "sfm_ambientocclusion_proxy_IMaterialProxy003",
  "sfm_ao_blur_proxy_IMaterialProxy003",
  "hsl_filmgrain_pass1_IMaterialProxy003",
  "hsl_filmgrain_pass2_IMaterialProxy003",
  "sfm_viewport_IMaterialProxy003",
  "sfm_blt_IMaterialProxy003",
  "Engine_Post_dx9_IMaterialProxy003",
  "VTOOLDICTIONARY003",
 };
 static bool admitted(const std::string&name)noexcept{
  if(name.size()>255||name.find('\0')!=std::string::npos)return false;
  for(unsigned char c:name)if(c>=128)return false;return true;
 }
 static const std::string_view*clientName(const std::string&name){
  auto key=sourceMaterialPatchLowerCandidate(name);
  for(const auto&registered:clientNames)if(sourceMaterialPatchLowerCandidate(std::string(registered))==key)return &registered;return nullptr;
 }
 static bool implemented(std::string_view name)noexcept{
  return name=="Equals"||name=="Sine"||name=="TextureTransform"||name=="TextureScroll"||name=="weapon_invis"||name=="Multiply"||name=="YellowLevel"||name=="ModelGlowColor";
 }
 static MaterialPinnedProxyClassificationCandidate classification(const std::string&name){
  if(!admitted(name))return MaterialPinnedProxyClassificationCandidate::Unverified;
  if(auto*p=clientName(name))return implemented(*p)?MaterialPinnedProxyClassificationCandidate::Implemented:MaterialPinnedProxyClassificationCandidate::RegisteredUnimplemented;
  const auto full=name+"_IMaterialProxy003";
  for(auto interfaceName:ifmInterfaces)if(interfaceName==full)return MaterialPinnedProxyClassificationCandidate::RegisteredUnimplemented;
  return MaterialPinnedProxyClassificationCandidate::OriginalFactoryMiss;
 }
 MaterialProxyInstanceCandidate*create(const std::string&name)override{
  if(!admitted(name))return nullptr;auto*canonical=clientName(name);
  return canonical&&implemented(*canonical)?MaterialModelGlowColorProxyFactoryCandidate::create(std::string(*canonical)):nullptr;
 }
 MaterialProxyMissingKindCandidate classifyMissing(const std::string&name)const override{
  return classification(name)==MaterialPinnedProxyClassificationCandidate::OriginalFactoryMiss?MaterialProxyMissingKindCandidate::OriginalFactoryMiss:MaterialProxyMissingKindCandidate::Unavailable;
 }
};
