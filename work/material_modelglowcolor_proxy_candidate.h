#pragma once
#include "material_yellowlevel_proxy_candidate.h"
// Installed ModelGlowColor and YellowLevel vtables share ALL five methods,
// including result Init and unconditional SetVec3(1,1,1) OnBind. Reuse the
// measured owned implementation; do not invent entity/color/time dependencies.
struct MaterialModelGlowColorProxyFactoryCandidate:MaterialYellowLevelProxyFactoryCandidate {
 MaterialProxyInstanceCandidate*create(const std::string&name)override{
  return name=="ModelGlowColor"?new MaterialYellowLevelProxyCandidate:MaterialYellowLevelProxyFactoryCandidate::create(name);
 }
};
