#pragma once
#include "model_skinning_geometry.h"
#include "render_camera_candidate.h"
#include <functional>
#include "vmt_keyvalues_candidate.h"
#include <filesystem>
struct RenderFrameStateCandidate {RenderCameraCandidate camera,placement;std::vector<GeometryMatrixCandidate>locals;bool visible=true;std::vector<std::pair<std::string,float>>materialAlpha;};
struct RenderFrameProviderCandidate {size_t count=0;uint32_t initialSkin=0,initialBody=0;std::string initialMaterialKey;std::function<void(const std::filesystem::path&,VmtNodeCandidate&)>applyMaterialOverrides;std::function<RenderFrameStateCandidate(size_t,const std::vector<GeometryBoneCandidate>&)>sample;};
