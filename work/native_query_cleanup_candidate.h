#pragma once
#include <d3d9.h>
#include <array>
#include "studio_query_cleanup_candidate.h"
// Resource's precise original interface is unresolved; only IUnknown::Release is used.
template<class Resource=IUnknown,class Query=IDirect3DQuery9>struct NativeQueryCleanupOwnerCandidateT{
 Resource*& resource;
 std::array<Query*,2>& queries;
 std::array<unsigned char,2>& active;
};
using NativeQueryCleanupOwnerCandidate=NativeQueryCleanupOwnerCandidateT<>;
template<class Lifecycle>struct NativeQueryCleanupServicesCandidateT{
 Lifecycle& lifecycle;
 bool diagnosticEnabled(){return lifecycle.diagnosticEnabled();}
 void diagnostic(){lifecycle.diagnostic();}
 double time(){return lifecycle.time();}
 double limit(){return lifecycle.limit();}
 template<class Object>void release(Object* value){value->Release();}
 template<class Query>HRESULT getData(Query* value,uint32_t& output,unsigned bytes,unsigned flags){return value->GetData(&output,bytes,flags);}
};
