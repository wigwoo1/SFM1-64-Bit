#pragma once
#include "apply_session_material_overrides_candidate.h"
#include <filesystem>
#include <functional>
inline std::function<void(const std::filesystem::path&,VmtNodeCandidate&)>sessionMaterialBindingCandidate(std::vector<SessionMaterialOverrideCandidate>records,std::vector<std::filesystem::path>mounts){
 return [records=std::move(records),mounts=std::move(mounts)](const auto&path,auto&syntax){
  std::string identity;
  for(const auto&mount:mounts){auto relative=path.lexically_normal().lexically_relative((mount/"materials").lexically_normal());if(relative.empty()||relative.is_absolute())continue;bool outside=false;for(const auto&part:relative)if(part=="..")outside=true;if(outside)continue;identity=normalizeSessionMaterialNameCandidate(relative.generic_string());break;}
  if(identity.empty())throw std::runtime_error("override VMT outside material mounts");
  for(const auto&record:records)if(record.material==identity){syntax=applySessionMaterialOverridesCandidate(std::move(syntax),record);return;}
 };
}
