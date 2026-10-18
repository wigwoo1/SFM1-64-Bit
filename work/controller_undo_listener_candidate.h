#pragma once
#include <cstdint>
template<class Child,class Adapter>
void updateControllerUndoChildCandidate(Child* child,std::uint32_t input,Adapter& a){
    const auto value=static_cast<std::uint8_t>(input);
    const bool changed=a.childByte15(child)!=value;
    a.setChildByte15(child,value);
    if(changed)a.invokeGlobalControllerChange(input);
}
template<class Adapter>
void controllerUndoListenerCandidate(Adapter& a){
    if(auto* child=a.globalChild344())updateControllerUndoChildCandidate(child,1,a);
    a.setListenerByte219(1);
    a.setListenerTimestamp228(a.readTimestampCounter());
}
