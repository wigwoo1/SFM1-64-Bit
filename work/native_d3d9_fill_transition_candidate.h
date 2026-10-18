#pragma once
#include <d3d9.h>
#include <array>
#include <cstdint>
#include <cstring>
// Dispatcher target1002e010, ordinary operation4. This record is the
// 72-byte transition state; it is distinct from shader shadow storage.
using NativeD3d9TransitionStateCandidate=std::array<uint8_t,72>;
inline uint32_t nativeD3d9FillTransitionValueCandidate(const NativeD3d9TransitionStateCandidate& state){
 uint32_t value;std::memcpy(&value,state.data()+12,4);return value;
}
inline std::array<uint32_t,3> nativeD3d9FillTransitionCommandCandidate(const NativeD3d9TransitionStateCandidate& state){
 return {1u,8u,nativeD3d9FillTransitionValueCandidate(state)};
}
inline HRESULT nativeD3d9FillTransitionCandidate(IDirect3DDevice9& device,const NativeD3d9TransitionStateCandidate& state){
 static_assert(D3DRS_FILLMODE==8,"native state binding");
 return device.SetRenderState(D3DRS_FILLMODE,nativeD3d9FillTransitionValueCandidate(state));
}
// Ordinary operation1/1002e070, flag byte68 bit1 -> render state14.
inline uint32_t nativeD3d9DepthWriteTransitionValueCandidate(const NativeD3d9TransitionStateCandidate& state){
 return (state[68]>>1)&1u;
}
inline HRESULT nativeD3d9DepthWriteTransitionCandidate(IDirect3DDevice9& device,const NativeD3d9TransitionStateCandidate& state){
 static_assert(D3DRS_ZWRITEENABLE==14,"native depth-write binding");
 return device.SetRenderState(D3DRS_ZWRITEENABLE,nativeD3d9DepthWriteTransitionValueCandidate(state));
}
// Ordinary operation2/1002e0e0. Cache publication deliberately reloads the
// live state after the setter callback, matching native owner+0x140 behavior.
template<class Setter,class OwnerGetter>void nativeD3d9ColorWriteTransitionOwnerCandidate(const NativeD3d9TransitionStateCandidate& state,Setter&& setter,OwnerGetter&& owner){
 uint32_t value;std::memcpy(&value,state.data()+8,4);
 static_assert(D3DRS_COLORWRITEENABLE==168,"native color-write binding");
 setter(D3DRS_COLORWRITEENABLE,value);
 std::memcpy(&value,state.data()+8,4);
 owner()=value;
}
template<class Setter>void nativeD3d9ColorWriteTransitionCandidate(const NativeD3d9TransitionStateCandidate& state,uint32_t& ownerCache,Setter&& setter){
 nativeD3d9ColorWriteTransitionOwnerCandidate(state,setter,[&]() ->uint32_t& {return ownerCache;});
}
