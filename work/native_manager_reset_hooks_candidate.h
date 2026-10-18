#pragma once
#include <d3d9.h>
// Registry supplies the recovered manager callback phase; hooks own other phases.
template<class Hooks,class Registry>struct NativeManagerResetHooksCandidateT{
 Hooks& hooks;Registry& registry;
 void managerCleanup(){registry.dispatch();}
 void ownerHook(bool enabled){hooks.ownerHook(enabled);}
 auto& commandState(){return hooks.commandState();}
 template<class State>void flush(State& state){hooks.flush(state);}
 template<class State>auto* device(State& state){return hooks.device(state);}
 void resetFailure(HRESULT result){hooks.resetFailure(result);}
 void postReset(bool a,bool b){hooks.postReset(a,b);}
};
