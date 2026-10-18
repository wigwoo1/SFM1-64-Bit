#pragma once
#include <d3d9.h>
#include <cstdint>
template<class Services>void studioPresentationMultisampleCandidate(D3DPRESENT_PARAMETERS& parameters,int32_t request,unsigned adapter,Services& services){
 if(request<=0||parameters.SwapEffect!=D3DSWAPEFFECT_DISCARD){parameters.MultiSampleType=D3DMULTISAMPLE_NONE;parameters.MultiSampleQuality=0;return;}
 unsigned sample=services.sampleType(request);
 const unsigned vendor=services.adapterVendor(adapter);
 const bool special=request==16&&vendor==0x10de;
 if(special)sample=4;
 unsigned quality=0;
 const auto result=services.sampleSupport(adapter,1u,parameters.BackBufferFormat,parameters.Windowed,static_cast<D3DMULTISAMPLE_TYPE>(sample),quality);
 if(static_cast<int32_t>(result)>=0){parameters.MultiSampleType=static_cast<D3DMULTISAMPLE_TYPE>(sample);parameters.MultiSampleQuality=special&&quality==16?15u:0u;}
}
