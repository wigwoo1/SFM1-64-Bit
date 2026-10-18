#pragma once
#include "native_d3d9_indexed_draw_candidate.h"
#include "studio_specialized_draw_factor_candidate.h"
// Primitive8/9 native body1000bb6b..1000bc0f. Caller must prepare
// native-compatible streams0/3/8 and supply the resolved integer factor.
// Float-factor conversion and resource preparation are deliberately separate.
inline NativeD3d9IndexedDrawCandidate nativeD3d9SpecializedPacketCandidate(uint32_t factor){
 return {5u,0,0,(factor+1u)*(factor+1u),0,2u*factor*(factor+2u)-4u};
}
inline void nativeD3d9SpecializedDrawCandidate(IDirect3DDevice9& device,int32_t rangeCount,uint32_t factor){
 device.SetStreamSourceFreq(0,static_cast<uint32_t>(rangeCount/4)|0x40000000u);
 device.SetStreamSourceFreq(3,0x80000001u);
 device.SetStreamSourceFreq(8,0x80000001u);
 nativeD3d9IndexedDrawCandidate(&device,nativeD3d9SpecializedPacketCandidate(factor));
 device.SetStreamSourceFreq(0,1);
 device.SetStreamSourceFreq(3,1);
 device.SetStreamSourceFreq(8,1);
}
inline void nativeD3d9SpecializedFiniteDrawCandidate(IDirect3DDevice9& device,int32_t rangeCount,float factor){
 nativeD3d9SpecializedDrawCandidate(device,rangeCount,studioSpecializedDrawFiniteFactorCandidate(factor));
}
