#pragma once
#include <cstdint>
template<class Adapter>void graphElementRefreshCandidate(Adapter& a){
 a.beginProfile();
 if(a.transient4e8()){
  if(auto* element=a.resolveModelSlot18(a.handle5c())){
   auto saved=static_cast<unsigned char>(a.recordingModelSlot61());a.setRecordingModelSlot60(0);
   a.transientSlot38(a.transient4e8(),0x3a83126fu,0);a.setRecordingModelSlot60(saved);
   auto* log=a.getLog10516a30(element,0);a.logSlot14(log,a.transient4e8());
   saved=static_cast<unsigned char>(a.recordingModelSlot61());a.setRecordingModelSlot60(0);
   a.elementSlot56(element);a.setRecordingModelSlot60(saved);
  }
  a.clearTransient4e8();a.afterTransient1073f430(0);
 }
 a.finalRefresh107358e0();a.endProfile();
}
