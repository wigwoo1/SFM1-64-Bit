#pragma once
#include "attribute_mutation_gate.h"
// Native object +0xB4, getter 0x1038F0D0. Hooks supply recovered subsystems.
struct MutationCycle{
 int phase=0;
 template<class Cleanup,class Initial> void shortCycle(Cleanup cleanup,Initial initial){
  if(phase==0){phase=1;cleanup();phase=2;initial();}phase=0;
 }
 template<class Cleanup,class Initial,class Build,class Evaluate,class Marked,class Remaining,class Finish>
 void fullCycle(Cleanup cleanup,Initial initial,Build build,Evaluate evaluate,Marked marked,Remaining remaining,Finish finish){
  if(phase==0){phase=1;cleanup();phase=2;initial();}
  phase=3;build();phase=4;evaluate();phase=5;marked();remaining();finish();phase=6;
 }
 bool permits(uint16_t flags)const{return attributeMutationPermitted(flags,phase);}
};
