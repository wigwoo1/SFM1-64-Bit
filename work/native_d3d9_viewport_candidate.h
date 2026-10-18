#pragma once
#include <d3d9.h>
#include <cstring>
#include "studio_shader_viewport_submit_candidate.h"
inline HRESULT nativeD3d9ViewportCandidate(IDirect3DDevice9& device,const StudioShaderViewportWordsCandidate& words){
 static_assert(sizeof(D3DVIEWPORT9)==24,"native viewport layout");
 D3DVIEWPORT9 viewport{};std::memcpy(&viewport,words.data(),sizeof(viewport));
 return device.SetViewport(&viewport);
}
