#pragma once
template<class Adapter>
inline void resetElementFrameworkPhaseCandidate(Adapter& a) {
    if(a.phase()==0) {
        a.setPhase(1);a.modelPrepare();
        a.setPhase(2);a.frameworkPrepare();
    }
    a.setPhase(0);
}
