#pragma once
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
// Slot208 valid-input policy; diagnostic identifies this range as flex weights.
// Owned boundary rejects negative/overflowing indices instead of emulating an
// unsafe native address calculation. Caller supplies >=1536 allocated registers.
template<class Services>bool studioExtendedVertexConstantsCandidate(
 int32_t start,int32_t count,const float* source,Services& services){
 if(services.vertexFloatCapacity()<1536)return false;
 if(start<0||start>512||count<0||int64_t{start}+count>(std::numeric_limits<int32_t>::max)())
  throw std::invalid_argument("invalid extended vertex constant range");
 if(start+count>512){services.warnClipped(512);count=512-start;}
 if(count<=0)return false;
 if(!source)throw std::invalid_argument("null extended vertex constant source");
 services.copy(services.staging()+std::size_t(start+1024)*4,source,static_cast<uint32_t>(count)*16);
 if(!(services.registrationFlags()&2)){
  // Copy remains published if append throws; flag stays clear so retry can
  // schedule again. Lifecycle must gate rendering until recovery completes.
  services.enqueueExtendedConstants();
  services.registrationFlags()|=2;
 }
 return true;
}
