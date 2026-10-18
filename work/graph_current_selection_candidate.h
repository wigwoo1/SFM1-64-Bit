#pragma once
template<class Adapter>auto graphCurrentSelectionCandidate(Adapter& a){
 auto* child=a.globalChild344();if(!child)return a.nullSelection();
 if(!a.resolveModelSlot18(a.childHandle80(child)))return a.nullSelection();
 auto* element=a.resolveModelSlot18(a.childHandle80(child));
 auto* first=a.lookup10002830(element);if(!first)return a.nullSelection();
 return a.lookup102c4420(first);
}
