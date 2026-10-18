#pragma once
#include "studio_pixel_color_candidate.h"
#include "studio_single_pixel_transport_candidate.h"
// Normalized state with borrowed full-width cache pointers. Service callbacks
// may mutate this owner; adapters must retain its reference, not a snapshot.
// Cache allocation, register capacity and lifetimes remain caller contracts.
struct StudioPixelColorOwnerCandidate {
 std::array<float,3> channels{};
 bool transformCaptured=false,suppressScale=false;
 float scale=1;
 uint32_t* current=nullptr;
 uint32_t* comparison=nullptr;
};
template<class Services>struct StudioPixelColorOwnerAdapterCandidate {
 StudioPixelColorOwnerCandidate& owner;Services& services;
 StudioPixelExposureCandidate exposure(){return services.exposure();}
 unsigned mode(){return services.mode();}
 float channel(unsigned index){return owner.channels[index];}
 bool transformCaptured(){return owner.transformCaptured;}
 bool usePower(){return services.usePower();}
 float transform(float value){return services.transform(value);}
 double power(double value,double exponent){return services.power(value,exponent);}
 bool suppressScale(){return owner.suppressScale;}
 unsigned serviceMode(){return services.serviceMode();}
 float scale(){return owner.scale;}
 uint32_t* currentCache(){return owner.current;}
 uint32_t* comparisonCache(){return owner.comparison;}
 void submitPixel(unsigned index,unsigned count,const uint32_t* words){services.submitPixel(index,count,words);}
};
template<class Services>bool studioPixelColorOwnerUploadCandidate(StudioPixelColorOwnerCandidate& owner,Services& services){
 StudioPixelColorOwnerAdapterCandidate<Services> adapter{owner,services};
 return studioPixelColorUploadCandidate(adapter);
}
template<class Backend>struct StudioPixelColorTransportCandidate : StudioSinglePixelTransportCandidate<Backend> {
 explicit StudioPixelColorTransportCandidate(Backend& backend):StudioSinglePixelTransportCandidate<Backend>{backend}{}
 StudioPixelExposureCandidate exposure(){return this->backend.exposure();}
 unsigned mode(){return this->backend.mode();}
 bool usePower(){return this->backend.usePower();}
 float transform(float value){return this->backend.transform(value);}
 double power(double value,double exponent){return this->backend.power(value,exponent);}
 unsigned serviceMode(){return this->backend.serviceMode();}
};
