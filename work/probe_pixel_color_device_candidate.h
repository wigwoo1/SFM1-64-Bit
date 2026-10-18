#pragma once
#include "studio_pixel_color_owner_candidate.h"
#include <stdexcept>
inline void probePixelColorDeviceCandidate(IDirect3DDevice9& device){
 std::array<float,4> original{};
 if(FAILED(device.GetPixelShaderConstantF(29,original.data(),1)))throw std::runtime_error("pixel original readback");
 struct Restore{IDirect3DDevice9& device;std::array<float,4>& values;bool active=true;~Restore(){if(active)device.SetPixelShaderConstantF(29,values.data(),1);}}restore{device,original};
 struct Ops{
  IDirect3DDevice9& device;unsigned calls=0;
  std::array<uint32_t,120> current{},comparison{};StudioDeviceQueueStateCandidate state{};
  StudioPixelExposureCandidate exposure(){return {2,true};}unsigned mode(){return 1;}
  bool usePower(){return false;}float transform(float){throw std::runtime_error("unexpected transform");}
  double power(double,double){throw std::runtime_error("unexpected power");}
  unsigned serviceMode(){return 1;}
  bool queueEnabled(){return false;}auto& queueState(){return state;}
  void publish(uint32_t*){throw std::runtime_error("unexpected publish");}
  uint32_t* acquire(unsigned){throw std::runtime_error("unexpected acquire");}
  void directPixel(unsigned index,unsigned count,const uint32_t* words){
   if(index!=29||count!=1)throw std::runtime_error("pixel routing");
   std::array<float,4> values{};std::memcpy(values.data(),words,16);
   if(FAILED(device.SetPixelShaderConstantF(index,values.data(),count)))throw std::runtime_error("pixel upload");++calls;
  }
 }ops{device};
 ops.comparison.fill(0xffffffff);
 StudioPixelColorOwnerCandidate owner{{.25f,.5f,.75f},false,false,2,ops.current.data(),ops.comparison.data()};
 StudioPixelColorTransportCandidate<Ops> services{ops};
 if(!studioPixelColorOwnerUploadCandidate(owner,services)||studioPixelColorOwnerUploadCandidate(owner,services)||ops.calls!=1)throw std::runtime_error("pixel cache reuse");
 std::array<float,4> actual{},expected{.5f,1.f,1.5f,.5f};
 if(FAILED(device.GetPixelShaderConstantF(29,actual.data(),1))||std::memcmp(actual.data(),expected.data(),16))throw std::runtime_error("pixel readback mismatch");
 if(FAILED(device.SetPixelShaderConstantF(29,original.data(),1)))throw std::runtime_error("pixel restore");
 restore.active=false;
}
