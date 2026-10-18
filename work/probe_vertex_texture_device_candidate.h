#pragma once
#include "studio_vertex_texture_submit_candidate.h"
#include "native_d3d9_vertex_texture_candidate.h"
#include "native_d3d9_texture_binding_candidate.h"
#include <memory>
#include <stdexcept>
#include <string>
inline void probeVertexTextureDeviceCandidate(IDirect3DDevice9& device){
 auto check=[](HRESULT hr){if(FAILED(hr))throw std::runtime_error("vertex texture device HRESULT "+std::to_string(uint32_t(hr)));};
 auto release=[](IDirect3DBaseTexture9* p){if(p)p->Release();};
 IDirect3DBaseTexture9* oldRaw=nullptr;check(device.GetTexture(257,&oldRaw));
 std::unique_ptr<IDirect3DBaseTexture9,decltype(release)> old(oldRaw,release);
 constexpr uint32_t types[]{1,2,6,5,7};DWORD oldSampler[5]{};
 for(unsigned i=0;i<5;++i)check(device.GetSamplerState(257,static_cast<D3DSAMPLERSTATETYPE>(types[i]),&oldSampler[i]));
 IDirect3DTexture9* raw=nullptr;check(device.CreateTexture(1,1,1,0,D3DFMT_R32F,D3DPOOL_MANAGED,&raw,nullptr));
 std::unique_ptr<IDirect3DBaseTexture9,decltype(release)> texture(raw,release);
 struct Ops {
  IDirect3DDevice9& device;unsigned textures=0,samplers=0;
  bool queueEnabled(){return false;}
  void queueTexture(uint32_t,uintptr_t){throw std::runtime_error("unexpected texture queue");}
  void queueSampler(uint32_t,uint32_t,uint32_t){throw std::runtime_error("unexpected sampler queue");}
  void directTexture(uint32_t s,uintptr_t p){if(FAILED(nativeD3d9VertexTextureCandidate(device,s,p)))throw std::runtime_error("vertex texture bind failed");++textures;}
  void directSampler(uint32_t s,uint32_t t,uint32_t v){if(FAILED(nativeD3d9VertexSamplerCandidate(device,s,t,v)))throw std::runtime_error("vertex sampler failed");++samplers;}
 } ops{device};
 StudioVertexTextureRecordCandidate record{{D3DTADDRESS_CLAMP,D3DTADDRESS_CLAMP,0,D3DTEXF_POINT,D3DTEXF_POINT,D3DTEXF_NONE},1,0,reinterpret_cast<uintptr_t>(raw)};
 auto* current=&record;StudioVertexTextureCacheCandidate cache{};
 studioVertexTextureSubmitCandidate(&current,&cache,1,1,ops);
 IDirect3DBaseTexture9* actualRaw=nullptr;check(device.GetTexture(257,&actualRaw));
 std::unique_ptr<IDirect3DBaseTexture9,decltype(release)> actual(actualRaw,release);
 if(actual.get()!=raw||ops.textures!=1||ops.samplers!=5)throw std::runtime_error("vertex texture binding readback mismatch");
 constexpr unsigned fields[]{0,1,4,3,5};
 for(unsigned i=0;i<5;++i){DWORD value=0;check(device.GetSamplerState(257,static_cast<D3DSAMPLERSTATETYPE>(types[i]),&value));if(value!=record.sampler[fields[i]])throw std::runtime_error("vertex sampler readback mismatch");}
 studioVertexTextureSubmitCandidate(&current,&cache,1,0,ops);
 if(ops.textures!=1||ops.samplers!=5)throw std::runtime_error("vertex texture reuse mismatch");
 current=nullptr;studioVertexTextureSubmitCandidate(&current,&cache,1,0,ops);
 IDirect3DBaseTexture9* unboundRaw=nullptr;check(device.GetTexture(257,&unboundRaw));
 std::unique_ptr<IDirect3DBaseTexture9,decltype(release)> unbound(unboundRaw,release);
 if(unbound||ops.textures!=2||ops.samplers!=5)throw std::runtime_error("vertex texture unbind mismatch");
 // Exercise the recovered general binding helper on the same owned resource.
 struct BindingPolicy {
  unsigned results=0;
  uint32_t budget(){return 0xffffffffu;}
  bool filteringEnabled(){return true;}bool mipEnabled(){return true;}
  uint32_t suppressed(){return 0;}
  void result(HRESULT hr){if(FAILED(hr))throw std::runtime_error("general texture binding bridge failed");++results;}
 } policy;
 NativeD3d9TextureBindingOpsCandidate<BindingPolicy> bindingOps{device,policy};
 DWORD oldAddressW=0;check(device.GetSamplerState(257,D3DSAMP_ADDRESSW,&oldAddressW));
 StudioTextureBindingRecordCandidate bindingRecord;
 bindingRecord.sampler=record.sampler;bindingRecord.sampler[2]=D3DTADDRESS_CLAMP;
 bindingRecord.resource=reinterpret_cast<uintptr_t>(raw);
 StudioTextureBindingStageCandidate bindingCache;bindingCache.enabled=1;bindingCache.sampler.fill(0xffffffffu);
 StudioTextureBindingOwnerCandidate bindingOwner;
 studioTextureBindingCandidate(257,&bindingRecord,1,bindingCache,bindingOwner,bindingOps);
 if(policy.results!=7)throw std::runtime_error("general texture bridge call count");
 IDirect3DBaseTexture9* boundRaw=nullptr;check(device.GetTexture(257,&boundRaw));
 std::unique_ptr<IDirect3DBaseTexture9,decltype(release)> bound(boundRaw,release);
 if(bound.get()!=raw)throw std::runtime_error("general texture bridge readback mismatch");
 constexpr uint32_t bindingTypes[]{1,2,3,6,5,7};
 constexpr unsigned bindingFields[]{0,1,2,4,3,5};
 for(unsigned i=0;i<6;++i){
  DWORD value=0;check(device.GetSamplerState(257,static_cast<D3DSAMPLERSTATETYPE>(bindingTypes[i]),&value));
  if(value!=bindingRecord.sampler[bindingFields[i]])throw std::runtime_error("general texture bridge sampler readback mismatch");
 }
 studioTextureBindingCandidate(257,&bindingRecord,0,bindingCache,bindingOwner,bindingOps);
 if(policy.results!=7)throw std::runtime_error("general texture bridge reuse mismatch");
 studioTextureBindingCandidate(257,nullptr,0,bindingCache,bindingOwner,bindingOps);
 if(policy.results!=8)throw std::runtime_error("general texture bridge unbind count");
 IDirect3DBaseTexture9* clearedRaw=nullptr;check(device.GetTexture(257,&clearedRaw));
 std::unique_ptr<IDirect3DBaseTexture9,decltype(release)> cleared(clearedRaw,release);
 if(cleared)throw std::runtime_error("general texture bridge null binding readback mismatch");
 check(device.SetSamplerState(257,D3DSAMP_ADDRESSW,oldAddressW));
 check(device.SetTexture(257,old.get()));
 for(unsigned i=0;i<5;++i)check(device.SetSamplerState(257,static_cast<D3DSAMPLERSTATETYPE>(types[i]),oldSampler[i]));
}
