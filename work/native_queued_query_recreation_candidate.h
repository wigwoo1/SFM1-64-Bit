#pragma once
#include <d3d9.h>
#include <array>
#include "studio_queued_query_recreation_candidate.h"
template<class Query=IDirect3DQuery9>struct NativeQueuedQueryOwnerCandidateT{
 int32_t& queryCapability;unsigned char& resourceQueueActive;std::array<Query*,20>& queuedResources;
};
template<class Lifecycle>struct NativeQueuedQueryServicesCandidateT{
 Lifecycle& lifecycle;
 auto commandState(){return lifecycle.commandState();}
 template<class State>void flush(State* state){lifecycle.flush(state);}
 template<class State>auto device(State* state){return lifecycle.device(state);}
 template<class Device,class Query>HRESULT createQuery(Device* device,unsigned type,Query*& output){return device->CreateQuery(static_cast<D3DQUERYTYPE>(type),&output);}
};
