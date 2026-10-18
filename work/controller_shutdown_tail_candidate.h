#pragma once
#include <cstdint>
template<class Adapter>
void controllerSubsystemReleaseCandidate(Adapter& a){
    if(!a.referenceCount())return;
    a.setReferenceCount(a.referenceCount()-1);
    if(a.referenceCount())return;
    a.cleanup2c90();
    if(a.subsystemFlags()&1)a.cleanup27f0();
    if(a.subsystemFlags()&2)a.cleanup0d50();
    a.setSubsystemFlags(0);
}
template<class Adapter>
void controllerShutdownTailCandidate(Adapter& a){
    if(auto* pool=a.threadPool()){
        a.invokePoolSlot5(pool,0xffffffff);
        a.destroyThreadPool(a.threadPool());
        a.clearThreadPool();
    }
    controllerSubsystemReleaseCandidate(a);
}
