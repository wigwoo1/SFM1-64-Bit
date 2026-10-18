#pragma once
#include <d3d9.h>
#include <cstdint>
#include "studio_vertex_texture_submit_candidate.h"
// Borrowed resource and device. No AddRef or release performed by submission.
inline HRESULT nativeD3d9VertexTextureCandidate(IDirect3DDevice9& device,uint32_t stage,uintptr_t resource){
 return device.SetTexture(stage,reinterpret_cast<IDirect3DBaseTexture9*>(resource));
}
inline HRESULT nativeD3d9VertexSamplerCandidate(IDirect3DDevice9& device,uint32_t stage,uint32_t type,uint32_t value){
 return device.SetSamplerState(stage,static_cast<D3DSAMPLERSTATETYPE>(type),value);
}

// Live device/mode references match the recovered per-submission reloads.
// Lifecycle owns queue encoding, capacity, resource retention and synchronization.
template<class Device,class Texture,class Lifecycle>
struct NativeVertexTextureSubmissionServicesCandidateT{
 Device*& currentDevice;uint32_t& queued;Lifecycle& lifecycle;
 bool queueEnabled(){return queued!=0;}
 void directTexture(uint32_t stage,uintptr_t resource){currentDevice->SetTexture(stage,reinterpret_cast<Texture*>(resource));}
 void directSampler(uint32_t stage,uint32_t type,uint32_t value){currentDevice->SetSamplerState(stage,static_cast<D3DSAMPLERSTATETYPE>(type),value);}
 void queueTexture(uint32_t stage,uintptr_t resource){lifecycle.queueTexture(stage,reinterpret_cast<Texture*>(resource));}
 void queueSampler(uint32_t stage,uint32_t type,uint32_t value){lifecycle.queueSampler(stage,static_cast<D3DSAMPLERSTATETYPE>(type),value);}
};
