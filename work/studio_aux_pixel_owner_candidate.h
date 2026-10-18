#pragma once
#include "studio_aux_pixel_constant_candidate.h"
#include "studio_single_pixel_transport_candidate.h"
// Normalized x64 state, not the original Source object layout. Cache storage
// is borrowed and must cover the register supplied by the caller. Services
// owns submission and its direct/queued transport policy. References are read
// live so service callbacks can replace caches and change calculation inputs.
struct StudioAuxPixelOwnerCandidate {
 uint32_t index=0xffffffffu;
 float lower=0,upper=0,second=0,density=0;
 uint32_t* current=nullptr;
 uint32_t* comparison=nullptr;
};
template<class Services>struct StudioAuxPixelOwnerAdapterCandidate {
 StudioAuxPixelOwnerCandidate& owner;
 Services& services;
 void publishIndex(uint32_t value){owner.index=value;}
 unsigned mode(){return services.mode();}
 float lower(){return owner.lower;}
 float upper(){return owner.upper;}
 float second(){return owner.second;}
 float density(){return owner.density;}
 uint32_t* currentCache(){return owner.current;}
 uint32_t* comparisonCache(){return owner.comparison;}
 void submitPixel(unsigned index,unsigned count,const uint32_t* words){services.submitPixel(index,count,words);}
};
template<class Services>bool studioAuxPixelOwnerUploadCandidate(
 StudioAuxPixelOwnerCandidate& owner,uint32_t index,uint32_t enabled,Services& services){
 StudioAuxPixelOwnerAdapterCandidate<Services> adapter{owner,services};
 return studioAuxPixelConstantUploadCandidate(index,enabled,adapter);
}
// Service bridge for the recovered single-register opcode7/direct transport.
// Backend supplies mode, queue state, publication/acquisition and directPixel.
template<class Backend>struct StudioAuxPixelTransportCandidate : StudioSinglePixelTransportCandidate<Backend> {
 explicit StudioAuxPixelTransportCandidate(Backend& backend):StudioSinglePixelTransportCandidate<Backend>{backend}{}
 unsigned mode(){return this->backend.mode();}
};
