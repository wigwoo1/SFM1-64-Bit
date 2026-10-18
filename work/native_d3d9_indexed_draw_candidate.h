#pragma once
#include <d3d9.h>
#include <cstdint>
#include <array>
#include "studio_mesh_primitive_count_candidate.h"
// Six argument words recovered from1000bae7 and queued opcode4.
// A logical packet, not a public Source ABI or serialized native queue node.
struct NativeD3d9IndexedDrawCandidate {
 uint32_t primitiveType=0;
 int32_t baseVertex=0;
 uint32_t minVertex=0,vertexCount=0,firstIndex=0,primitiveCount=0;
};
inline NativeD3d9IndexedDrawCandidate nativeD3d9IndexedPacketCandidate(uint32_t type,int32_t baseVertex,uint32_t minVertex,uint32_t vertices,uint32_t first,uint32_t indices){
 return {type,baseVertex,minVertex,vertices,first,studioMeshPrimitiveCountCandidate(type,vertices,indices)};
}
// Signed baseVertex is serialized modulo2^32, preserving its native bits.
// No pointers occur in opcode4; queue ownership/publication remains separate.
inline std::array<uint32_t,7> nativeD3d9IndexedCommandCandidate(const NativeD3d9IndexedDrawCandidate& draw){
 return {4u,draw.primitiveType,static_cast<uint32_t>(draw.baseVertex),draw.minVertex,draw.vertexCount,draw.firstIndex,draw.primitiveCount};
}
// Caller retains the device and prepares streams, indices, shaders and render
// state before this synchronous call. HRESULT is exposed for caller diagnostics.
inline HRESULT nativeD3d9IndexedDrawCandidate(IDirect3DDevice9* device,const NativeD3d9IndexedDrawCandidate& draw){
 if(!device)return E_POINTER; // Prototype policy for absent device.
 return device->DrawIndexedPrimitive(static_cast<D3DPRIMITIVETYPE>(draw.primitiveType),draw.baseVertex,draw.minVertex,draw.vertexCount,draw.firstIndex,draw.primitiveCount);
}
