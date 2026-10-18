#pragma once
// Native interface/window access; original imported cleanup remains explicit.
template<class Adapter>
void cleanupControllerListenerCandidate(Adapter& a){
    a.removePostActionListener(a.nativeListenerInterface());
    a.closeAllWindows();
    a.flushApplication();
    if(auto* window=a.window())a.invokeWindowSlot3(window,1);
    a.clearWindow();
    a.cleanup25e0();
    a.cleanupTail2f1700();
}
