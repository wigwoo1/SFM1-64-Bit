#pragma once
#include <d3d9.h>
#include <cstdint>
#include <array>
// Native mesh primitive0/10 branch1000bc0f: queued opcode3 or COMslot81.
// The count is passed directly from the range; no indexed-count conversion.
struct NativeD3d9NonindexedDrawCandidate {
 uint32_t primitiveType=0,firstVertex=0,primitiveCount=0;
};
// Fixed-width native command payload. Publication and the conditional flush
// following opcode3 are separate operations; this does not execute a queue.
inline std::array<uint32_t,4> nativeD3d9NonindexedCommandCandidate(const NativeD3d9NonindexedDrawCandidate& draw){
 return {3u,draw.primitiveType,draw.firstVertex,draw.primitiveCount};
}
// Caller retains device and binds vertex streams, shaders and render state.
inline HRESULT nativeD3d9NonindexedDrawCandidate(IDirect3DDevice9* device,const NativeD3d9NonindexedDrawCandidate& draw){
 if(!device)return E_POINTER;
 return device->DrawPrimitive(static_cast<D3DPRIMITIVETYPE>(draw.primitiveType),draw.firstVertex,draw.primitiveCount);
}
