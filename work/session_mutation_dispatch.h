#pragma once
#include "session_runtime.h"
#include "mutation_coordinator.h"
// Queue lookup uses generation-checked session handles, matching native drain.
struct SessionMutationDispatch{
 SessionRuntime& runtime;MutationCoordinator& mutation;
 // Native framework +0xA0/+0xAC: persistent listener vector, pointer-width here.
 std::vector<uintptr_t> listeners;
 // Native phase-four vector +0x50/+0x5C, distinct from drain listeners.
 std::vector<uintptr_t> evaluationListeners;
 template<class Evaluate> void evaluate(Evaluate dispatch){size_t count=evaluationListeners.size();for(size_t i=0;i<count;++i)dispatch(evaluationListeners[i]);}
 // Native 0x1038F650 appends without uniqueness or null validation.
 void addListener(uintptr_t listener){listeners.push_back(listener);}
 // Cleanup, dependency build and finish remain explicit unrecovered subsystems.
 template<class Cleanup,class Build,class Evaluate,class Notify,class General,class Attribute,class Listener,class Finish>
 void fullCycle(Cleanup cleanup,Build build,Evaluate evaluation,Notify notify,General general,Attribute attribute,Listener listener,Finish finish){
  mutation.cycle.fullCycle(cleanup,[&](){drain(notify,general,attribute,listener);},build,[&](){evaluate(evaluation);},[&](){processMarked(notify);},[&](){remaining(general,attribute,listener);},finish);
 }
 bool live(uint32_t handle)const{return runtime.elements.resolve(runtime.graph,handle)!=nullptr;}
 void processPropagated(){mutation.processPropagatedLive([&](uint32_t h){return live(h);});}
 template<class Notify> void processMarked(Notify notify){mutation.processMarkedLive([&](uint32_t h){return live(h);},notify);}
 template<class General,class Attribute> void processPrepared(General general,Attribute attribute){mutation.processPreparedLive([&](uint32_t h){return live(h);},general,attribute);}
 // Native 0x1038F210: marked, prepared, listeners, propagated. Listener count
 // is captured after prepared dispatch. Appends during listeners wait a drain.
 // Callbacks may append listeners; removal during dispatch is not supported yet.
 template<class Notify,class General,class Attribute,class Listener>
 void drain(Notify notify,General general,Attribute attribute,Listener listener){
  processMarked(notify);remaining(general,attribute,listener);
 }
 // Native 0x1038F160, also used after marked dispatch in the full cycle.
 template<class General,class Attribute,class Listener>
 void remaining(General general,Attribute attribute,Listener listener){
  processPrepared(general,attribute);
  size_t count=listeners.size();for(size_t i=0;i<count;++i)listener(listeners[i]);
  processPropagated();
 }
};
