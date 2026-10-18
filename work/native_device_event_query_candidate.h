#pragma once
#include <d3d9.h>
#include "studio_device_event_query_candidate.h"
template<class Device,class Query=IDirect3DQuery9>struct NativeDeviceEventQueryBackendCandidateT{
 Device& device;Query* initialOutput(){return nullptr;}
 HRESULT create(unsigned type,Query*& output){return device.CreateQuery(static_cast<D3DQUERYTYPE>(type),&output);}
 void release(Query* query){query->Release();}
};
using NativeDeviceEventQueryBackendCandidate=NativeDeviceEventQueryBackendCandidateT<IDirect3DDevice9>;
