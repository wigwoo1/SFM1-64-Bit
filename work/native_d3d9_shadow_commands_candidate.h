#pragma once
#include "native_shader_shadow_snapshot_candidate.h"
#include <d3d9.h>
#include <vector>
struct NativeD3d9ShadowCommandCandidate {D3DRENDERSTATETYPE state;DWORD value;};
// API application of recovered fields; native transition scheduling and
// alpha-test-enable policy, culling, shader bindings and samplers are separate.
inline std::vector<NativeD3d9ShadowCommandCandidate> nativeD3d9ShadowCommandsCandidate(const NativeShaderShadowSnapshotCandidate&storage){
 auto read=[&](size_t offset){uint32_t value;std::memcpy(&value,storage.data()+offset,4);return value;};
 if(read(0x28)<1||read(0x28)>8||read(0x2c)>1||read(0x50)<1||read(0x50)>8||read(0x54)>255)throw std::runtime_error("D3D9 depth/alpha field range");
 for(size_t offset:{size_t(0x38),size_t(0x3c),size_t(0x44),size_t(0x48)})if(read(offset)<1||read(offset)>11)throw std::runtime_error("D3D9 blend factor range");
 for(size_t offset:{size_t(0x40),size_t(0x4c)})if(read(offset)<1||read(offset)>5)throw std::runtime_error("D3D9 blend operation range");
 return {{D3DRS_ZENABLE,static_cast<DWORD>((storage[0x6c]&2u)?D3DZB_TRUE:D3DZB_FALSE)},{D3DRS_ZWRITEENABLE,read(0x2c)},{D3DRS_ZFUNC,read(0x28)},{D3DRS_COLORWRITEENABLE,read(0x30)&15u},{D3DRS_ALPHABLENDENABLE,(storage[0x6c]&32u)?1u:0u},{D3DRS_SRCBLEND,read(0x38)},{D3DRS_DESTBLEND,read(0x3c)},{D3DRS_BLENDOP,read(0x40)},{D3DRS_SEPARATEALPHABLENDENABLE,(storage[0x6d]&1u)?1u:0u},{D3DRS_SRCBLENDALPHA,read(0x44)},{D3DRS_DESTBLENDALPHA,read(0x48)},{D3DRS_BLENDOPALPHA,read(0x4c)},{D3DRS_ALPHAFUNC,read(0x50)},{D3DRS_ALPHAREF,read(0x54)}};
}
template<class Setter> inline void applyNativeD3d9ShadowCommandsCandidate(const NativeShaderShadowSnapshotCandidate&storage,Setter&&setter){auto commands=nativeD3d9ShadowCommandsCandidate(storage);for(const auto&command:commands)if(FAILED(setter(command.state,command.value)))throw std::runtime_error("D3D9 shadow state application failed");}
