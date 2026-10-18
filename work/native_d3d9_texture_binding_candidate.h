#pragma once
#include "studio_texture_binding_candidate.h"
#include "studio_sampler_submit_candidate.h"
#include "native_d3d9_vertex_texture_candidate.h"
#include <stdexcept>
// Direct device bridge; Policy supplies live suppression/configuration values.
// The caller owns device/resources and can observe HRESULTs without altering
// native cache-publication timing. Native queue transport remains separate.
template<class Policy>struct NativeD3d9TextureBindingOpsCandidate {
 IDirect3DDevice9& device;Policy& policy;
 uint32_t budget(){return policy.budget();}
 bool filteringEnabled(){return policy.filteringEnabled();}
 bool mipEnabled(){return policy.mipEnabled();}
 uint32_t suppressed(){return policy.suppressed();}
 bool queueEnabled(){return false;}
 void texture(uint32_t stage,uintptr_t resource){policy.result(nativeD3d9VertexTextureCandidate(device,stage,resource));}
 void sampler(uint32_t stage,uint32_t type,uint32_t value){studioSamplerSubmitCandidate(stage,type,value,*this);}
 void directSampler(uint32_t stage,uint32_t type,uint32_t value){policy.result(nativeD3d9VertexSamplerCandidate(device,stage,type,value));}
 void queueSampler(const std::array<uint32_t,4>&){throw std::logic_error("direct texture bridge queue requested");}
};
