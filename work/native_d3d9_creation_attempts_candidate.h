#pragma once
#include <d3d9.h>
#include "studio_d3d9_creation_attempts_candidate.h"
inline void nativeD3d9PresentationFallbackCandidate(D3DPRESENT_PARAMETERS& parameters){
 parameters.SwapEffect=D3DSWAPEFFECT_COPY;
 parameters.BackBufferCount=0;
}
// Real device-call bridge; earlier Source flag policy and failure UI external.
// Borrow the interface pointer slot; native code reloads it on each attempt.
template<class Api>struct NativeD3d9CreationAttemptsBackendCandidateT {
 Api*& api;UINT adapter;D3DDEVTYPE type;HWND window;DWORD flags;
 D3DPRESENT_PARAMETERS& parameters;
 HRESULT lastResult=E_FAIL;unsigned attempts=0,waits=0,fallbacks=0;
 IDirect3DDevice9* initialOutput(){return nullptr;}
 void configureFlags(uint32_t value){flags=value;}
 HRESULT create(IDirect3DDevice9*& output){
  ++attempts;lastResult=api->CreateDevice(adapter,type,window,flags,&parameters,&output);return lastResult;
 }
 void wait(unsigned ms){++waits;Sleep(ms);}
 bool fallbackEnabled(){return parameters.Windowed!=FALSE;}
 void configureFallback(){++fallbacks;nativeD3d9PresentationFallbackCandidate(parameters);}
 void failure(){}
};
using NativeD3d9CreationAttemptsBackendCandidate=NativeD3d9CreationAttemptsBackendCandidateT<IDirect3D9>;
