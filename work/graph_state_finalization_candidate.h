#pragma once
template<class Adapter>void graphStateFinalizationCandidate(Adapter& a){
 a.setByte9(0);
 if(a.state10()){
  a.beforeUndo100c09b0();auto* object=a.allocateNativeUndo();
  if(object)object=a.construct100bf820(object,a.owner4());
  a.submitModelSlot79(object);a.endModelSlot67();
 }
 a.setState10(0);
}
