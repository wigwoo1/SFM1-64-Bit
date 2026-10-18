#pragma once
#include <d3d9.h>
// Invalid state is separate from a shader pointer; never encode an invalidation
// sentinel as a dereferenceable x64 interface pointer.
template<class Shader>struct NativeShaderBindingStateCandidateT{
 Shader* shader=nullptr;bool valid=false;
 void invalidate(){valid=false;}
};
template<bool Vertex,class Shader,class Lifecycle>
bool nativeShaderBindingCandidate(NativeShaderBindingStateCandidateT<Shader>& cache,
 Shader* shader,Lifecycle& lifecycle){
 if(cache.valid&&cache.shader==shader)return false;
 lifecycle.recordBind(Vertex);
 auto command=lifecycle.commandState();
 if(lifecycle.queueEnabled(command))lifecycle.queueShader(command,Vertex,shader);
 else{
  // Device is retrieved after command-state capture; lifetime belongs to caller.
  if constexpr(Vertex)lifecycle.device(command)->SetVertexShader(shader);
  else lifecycle.device(command)->SetPixelShader(shader);
 }
 // Original low-level publication happens even on a failed HRESULT.
 cache.shader=shader;cache.valid=true;return true;
}
template<class VertexShader,class PixelShader,class Lifecycle>
void nativeShaderRestoreUnbindCandidate(NativeShaderBindingStateCandidateT<VertexShader>& vertex,
 NativeShaderBindingStateCandidateT<PixelShader>& pixel,Lifecycle& lifecycle){
 vertex.invalidate();pixel.invalidate();
 nativeShaderBindingCandidate<true>(vertex,static_cast<VertexShader*>(nullptr),lifecycle);
 nativeShaderBindingCandidate<false>(pixel,static_cast<PixelShader*>(nullptr),lifecycle);
}
