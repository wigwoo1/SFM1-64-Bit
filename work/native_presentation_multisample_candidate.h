#pragma once
#include <d3d9.h>
#include "studio_presentation_multisample_candidate.h"
// Borrow the live API slot; selector/vendor callbacks may replace its receiver.
template<class Api,class Lifecycle>struct NativePresentationMultisampleServicesCandidateT{
 Api*& api;
 Lifecycle& lifecycle;
 template<class Display>void displayMode(Display& value,unsigned adapter){lifecycle.displayMode(value,adapter);}
 template<class Display,class Descriptor>void dimensions(Display& value,Descriptor& descriptor,int32_t& width,int32_t& height){lifecycle.dimensions(value,descriptor,width,height);}
 float aspectThreshold(){return lifecycle.aspectThreshold();}
 auto& material(){return lifecycle.material();}
 bool forceNoVsync(){return lifecycle.forceNoVsync();}
 bool tripleBuffered(){return lifecycle.tripleBuffered();}
 unsigned colorChoice(unsigned adapter,unsigned type,unsigned prior,unsigned requested,unsigned windowed){return lifecycle.colorChoice(adapter,type,prior,requested,windowed);}
 unsigned depthFormat(unsigned adapter,unsigned prior,unsigned color,unsigned preferred){return lifecycle.depthFormat(adapter,prior,color,preferred);}
 unsigned translateFormat(unsigned color){return lifecycle.translateFormat(color);}
 unsigned sampleType(int32_t request){return lifecycle.sampleType(request);}
 unsigned adapterVendor(unsigned adapter){return lifecycle.adapterVendor(adapter);}
 HRESULT sampleSupport(unsigned adapter,unsigned type,D3DFORMAT format,BOOL windowed,D3DMULTISAMPLE_TYPE sample,unsigned& output){
  DWORD quality=output;
  const HRESULT result=api->CheckDeviceMultiSampleType(adapter,static_cast<D3DDEVTYPE>(type),format,windowed,sample,&quality);
  output=static_cast<unsigned>(quality);
  return result;
 }
};
template<class Lifecycle>using NativePresentationMultisampleServicesCandidate=NativePresentationMultisampleServicesCandidateT<IDirect3D9,Lifecycle>;
