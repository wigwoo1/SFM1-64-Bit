#pragma once
#include <d3d9.h>
#include <cstdint>
static_assert(D3DUSAGE_DYNAMIC==0x200&&D3DFMT_A8R8G8B8==21&&D3DPOOL_DEFAULT==0);
template<class Owner,class Services>
void studioResetTextureRecreationCandidate(Owner& owner,Services& services){
 if(owner.resource){services.release(owner.resource);owner.resource=nullptr;}
 auto* state=services.commandState();services.flush(state);auto* device=services.device(state);
 if(static_cast<int32_t>(services.createTexture(device,owner.resource))<0)owner.resource=nullptr;
}
template<class Lifecycle>struct NativeResetTextureServicesCandidateT{
 Lifecycle& lifecycle;
 template<class Texture>void release(Texture* texture){texture->Release();}
 auto commandState(){return lifecycle.commandState();}
 template<class State>void flush(State* state){lifecycle.flush(state);}
 template<class State>auto device(State* state){return lifecycle.device(state);}
 template<class Device,class Texture>HRESULT createTexture(Device* device,Texture*& output){return device->CreateTexture(1,1,0,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&output,nullptr);}
};

