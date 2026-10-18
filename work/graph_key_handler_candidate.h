#pragma once
template<class Event,class Adapter>void graphKeyHandlerCandidate(Event* event,Adapter& a){
 if(a.eventFlags1c(event)&1)return;
 auto global=a.cursorPosition();auto local=a.mapFromGlobal(global);
 auto modifiers=a.eventModifiers(event);auto buttons=a.mouseButtons();
 a.dispatch100d35b0(local,modifiers,buttons,0);
}
