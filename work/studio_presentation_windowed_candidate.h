#pragma once
#include <d3d9.h>
#include <cstdint>
#include <stdexcept>
#include "studio_presentation_multisample_candidate.h"
// Normalized input fields; this does not define the original descriptor ABI.
struct StudioPresentationDescriptorCandidate {
 int32_t width,height;
 unsigned colorFormat;
 int32_t depthPreference;
 unsigned char flags;
 int32_t limitWidth,limitHeight;
 int32_t sampleRequest;
 int32_t refreshNumerator=0,refreshDenominator=0;
};
template<class Services>void studioPresentationWindowedCandidate(
 D3DPRESENT_PARAMETERS& parameters,unsigned char& ownerFlags,unsigned& depthBits,
 HWND window,unsigned adapter,unsigned priorFormat,StudioPresentationDescriptorCandidate& descriptor,
 int32_t displayWidth,int32_t displayHeight,Services& services){
 parameters={};parameters.Windowed=descriptor.flags&1u;
 parameters.SwapEffect=static_cast<D3DSWAPEFFECT>((descriptor.flags|0x40u)>>6);
 parameters.EnableAutoDepthStencil=TRUE;
 const auto color=services.colorChoice(adapter,1u,priorFormat,descriptor.colorFormat,descriptor.flags&1u);
 ownerFlags=static_cast<unsigned char>((ownerFlags&0xefu)|((descriptor.flags<<2)&0x10u));
 if(descriptor.depthPreference>=80)ownerFlags|=0x10u;
 const auto depth=services.depthFormat(adapter,priorFormat,color,(ownerFlags&0x10u)?75u:77u);
 parameters.AutoDepthStencilFormat=static_cast<D3DFORMAT>(depth);parameters.hDeviceWindow=window;
 switch(depth){case 73:depthBits=1;break;case 75:depthBits=8;break;case 79:depthBits=4;break;default:ownerFlags&=0xefu;depthBits=0;}
 if(!(descriptor.flags&1u)){
  const bool explicitSize=descriptor.width!=0&&descriptor.height!=0;
  parameters.BackBufferWidth=explicitSize?descriptor.width:displayWidth;
  parameters.BackBufferHeight=explicitSize?descriptor.height:displayHeight;
  parameters.BackBufferFormat=static_cast<D3DFORMAT>(services.translateFormat(color));
  if((descriptor.flags&0x10u)&&!services.forceNoVsync()){
   const bool triple=services.tripleBuffered();
   parameters.PresentationInterval=1;
   parameters.BackBufferCount=triple?2u:1u;
  }else{parameters.BackBufferCount=1;parameters.PresentationInterval=0x80000000u;}
  if(descriptor.refreshDenominator){
   if(descriptor.refreshNumerator==INT32_MIN&&descriptor.refreshDenominator==-1)throw std::overflow_error("native refresh division overflow");
   parameters.FullScreen_RefreshRateInHz=descriptor.refreshNumerator/descriptor.refreshDenominator;
  }
 }else{
 parameters.PresentationInterval=0x80000000u;
 if(descriptor.flags&2u){
  if((descriptor.flags&8u)&&(descriptor.limitWidth<displayWidth||descriptor.limitHeight<displayHeight)){
   parameters.BackBufferWidth=descriptor.limitWidth;parameters.BackBufferHeight=descriptor.limitHeight;
  }else{parameters.BackBufferWidth=displayWidth;parameters.BackBufferHeight=displayHeight;}
 }else{parameters.BackBufferWidth=descriptor.width;parameters.BackBufferHeight=descriptor.height;}
 parameters.BackBufferFormat=static_cast<D3DFORMAT>(services.translateFormat(color));parameters.BackBufferCount=1;
 }
 studioPresentationMultisampleCandidate(parameters,descriptor.sampleRequest,adapter,services);
}
