#pragma once
// Recovered service setup100290a0/release100290e0. Interface is full-width and
// backend-owned. Callbacks supply base setup, factory, warning and base cleanup.
// Reinitialization/lifetime preconditions remain the caller's responsibility.
template<class Interface,class Backend>bool studioD3D9FactorySetupCandidate(
 Interface*& current,Backend& backend){
 if(!static_cast<unsigned char>(backend.baseSetup()))return false;
 current=backend.create(32);
 if(!current){backend.factoryFailure();return false;}
 return true;
}
template<class Interface,class Backend>void studioD3D9FactoryReleaseCandidate(
 Interface*& current,Backend& backend){
 auto* captured=current;
 if(captured){backend.release(captured);current=nullptr;}
 backend.baseCleanup();
}
