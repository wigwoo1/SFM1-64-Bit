#include "native_d3d9_indexed_draw_candidate.h"
#include "native_d3d9_nonindexed_draw_candidate.h"
#include "native_d3d9_specialized_draw_candidate.h"
#include "native_d3d9_fill_transition_candidate.h"
// Compiles the full device-call path; this file provides no runtime GPU proof.
HRESULT compileNativeD3d9IndexedDraw(IDirect3DDevice9* device,const NativeD3d9IndexedDrawCandidate& packet){return nativeD3d9IndexedDrawCandidate(device,packet);}
HRESULT compileNativeD3d9NonindexedDraw(IDirect3DDevice9* device,const NativeD3d9NonindexedDrawCandidate& packet){return nativeD3d9NonindexedDrawCandidate(device,packet);}
std::array<uint32_t,7> compileNativeD3d9IndexedCommand(const NativeD3d9IndexedDrawCandidate& packet){return nativeD3d9IndexedCommandCandidate(packet);}
std::array<uint32_t,4> compileNativeD3d9NonindexedCommand(const NativeD3d9NonindexedDrawCandidate& packet){return nativeD3d9NonindexedCommandCandidate(packet);}
void compileNativeD3d9SpecializedDraw(IDirect3DDevice9& device,int32_t rangeCount,uint32_t factor){nativeD3d9SpecializedDrawCandidate(device,rangeCount,factor);}
void compileNativeD3d9SpecializedFiniteDraw(IDirect3DDevice9& device,int32_t rangeCount,float factor){nativeD3d9SpecializedFiniteDrawCandidate(device,rangeCount,factor);}
HRESULT compileNativeD3d9FillTransition(IDirect3DDevice9& device,const NativeD3d9TransitionStateCandidate& state){return nativeD3d9FillTransitionCandidate(device,state);}
HRESULT compileNativeD3d9DepthWriteTransition(IDirect3DDevice9& device,const NativeD3d9TransitionStateCandidate& state){return nativeD3d9DepthWriteTransitionCandidate(device,state);}
void compileNativeD3d9ColorWriteTransition(IDirect3DDevice9& device,const NativeD3d9TransitionStateCandidate& state,uint32_t& cache){nativeD3d9ColorWriteTransitionCandidate(state,cache,[&](D3DRENDERSTATETYPE key,DWORD value){return device.SetRenderState(key,value);});}
