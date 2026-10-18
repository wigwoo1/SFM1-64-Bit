#include <initializer_list>
#include "native_reset_texture_recreation_candidate.h"
#include <cassert>
#include <cstdio>
struct Texture{unsigned releases=0;ULONG Release(){++releases;return 0;}};
struct Device{HRESULT result;Texture* output;unsigned calls=0;HRESULT CreateTexture(UINT w,UINT h,UINT levels,DWORD usage,D3DFORMAT fmt,D3DPOOL pool,Texture** out,HANDLE* shared){assert(w==1&&h==1&&levels==0&&usage==D3DUSAGE_DYNAMIC&&fmt==D3DFMT_A8R8G8B8&&pool==D3DPOOL_DEFAULT&&!shared);++calls;*out=output;return result;}};
struct State{Device* device;};struct Lifecycle{State* state;Device* replacement;bool replace;unsigned flushes=0;State* commandState(){return state;}void flush(State* s){assert(s==state);++flushes;if(replace)s->device=replacement;}Device* device(State* s){assert(flushes==1);return s->device;}};struct Owner{Texture* resource;};
struct SdkLifecycle{IDirect3DDevice9** state;IDirect3DDevice9** commandState(){return state;}void flush(IDirect3DDevice9**){}IDirect3DDevice9* device(IDirect3DDevice9** s){return *s;}};
void sdkInstantiate(IDirect3DTexture9*& texture,SdkLifecycle& lifecycle){struct View{IDirect3DTexture9*& resource;}owner{texture};NativeResetTextureServicesCandidateT<SdkLifecycle> services{lifecycle};studioResetTextureRecreationCandidate(owner,services);}
int main(){unsigned cases=0;for(bool present:{false,true})for(bool replace:{false,true})for(HRESULT result:{S_OK,S_FALSE,E_FAIL,static_cast<HRESULT>(0x80000000u),static_cast<HRESULT>(0x7fffffffu)}){Texture old{},fresh{};Device first{result,&fresh},second{result,&fresh};State state{&first};Lifecycle lifecycle{&state,&second,replace};Owner owner{present?&old:nullptr};NativeResetTextureServicesCandidateT<Lifecycle> services{lifecycle};studioResetTextureRecreationCandidate(owner,services);assert(old.releases==(present?1u:0u));assert(first.calls==(replace?0u:1u)&&second.calls==(replace?1u:0u));assert(owner.resource==(result<0?nullptr:&fresh));++cases;}std::printf("reset_texture_recreation_cases=%u passed\n",cases);}

