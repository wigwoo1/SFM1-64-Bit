#include "graph_undo_methods_candidate.h"
#include <cassert>
#include <iostream>
struct Owner{unsigned counter;};struct Object{Owner* owner;};struct Adapter{Object* object;Owner replacement{0};unsigned calls=0;
 Owner* owner14(Object* p){return p->owner;}unsigned counter29c(Owner* p){return p->counter;}void setCounter29c(Owner* p,unsigned n){p->counter=n;}
 void serviceSlot10(){++calls;object->owner=&replacement;}
};
int main(){static_assert(sizeof(void*)==8);Owner owner{0xffffffff};Object object{&owner};Adapter a{&object};graphUndoSlot0Candidate(&object,a);assert(owner.counter==0 && !a.calls);graphUndoSlot1Candidate(&object,a);assert(a.calls==1 && object.owner==&a.replacement && a.replacement.counter==0xffffffff && owner.counter==0);graphUndoSlot0Candidate(&object,a);assert(!a.replacement.counter);std::cout<<"x64 graph undo virtual methods: wrapped counter changes and owner reload after service callback passed\n";}
