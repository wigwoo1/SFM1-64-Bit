#pragma once
#include <array>
#include "native_reset_texture_recreation_candidate.h"
#include "native_queued_query_recreation_candidate.h"
#include "studio_pair_query_recreation_candidate.h"
template<class Texture=IDirect3DTexture9,class Query=IDirect3DQuery9>
struct NativePairQueryOwnerCandidateT{
 int32_t& queryCapability;Texture*& resource;std::array<Query*,2>& queries;std::array<unsigned char,2>& active;
};
template<class Lifecycle>struct NativePairQueryServicesCandidateT:NativeQueuedQueryServicesCandidateT<Lifecycle>{
 explicit NativePairQueryServicesCandidateT(Lifecycle& lifecycle):NativeQueuedQueryServicesCandidateT<Lifecycle>{lifecycle}{}
 bool diagnosticEnabled(){return this->lifecycle.diagnosticEnabled();}
 void diagnostic(){this->lifecycle.diagnostic();}
 void queryUnavailable(){this->lifecycle.queryUnavailable();}
 template<class Owner>void recreateTexture(Owner& owner){NativeResetTextureServicesCandidateT<Lifecycle> services{this->lifecycle};studioResetTextureRecreationCandidate(owner,services);}
 template<class Query>void issue(Query* query,unsigned flags){query->Issue(flags);}
};
