#pragma once
template<class Adapter>void graphPrefinalizationCandidate(Adapter& a){
 a.beginImportedScope();
 if(auto* collection=a.ownerCollection14()){
  const auto count=a.collectionCount24(collection);
  for(int i=0;i<count;++i)a.releaseRecordNested18(collection,i);
  a.clearOwnerCollection14();
 }
 auto* state=a.currentGraphState();const auto count=a.stateCount84(state);
 for(int i=0;i<count;++i){auto* element=a.resolveModelSlot18(a.stateHandle78(state,i));if(element && static_cast<unsigned char>(a.filterElementSlot4(element)))a.refresh10741340(element);}
 if(auto* child=a.liveChild344())a.updateChild10275f50(child);
 a.endImportedScope();
}
