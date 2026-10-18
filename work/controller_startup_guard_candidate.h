#pragma once
template<class Adapter>bool initializeControllerStartupGuardCandidate(Adapter& a){
 if(a.initialized83c())return true;
 a.setInitialized83c(1);
 a.initialize30180();
 if(!a.initialize30020())return false;
 a.initialize30010();a.initialize30000();return true;
}
