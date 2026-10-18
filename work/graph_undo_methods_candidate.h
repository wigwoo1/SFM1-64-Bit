#pragma once
template<class Object,class Adapter>void graphUndoSlot0Candidate(Object* object,Adapter& a){auto* owner=a.owner14(object);a.setCounter29c(owner,a.counter29c(owner)+1u);}
template<class Object,class Adapter>void graphUndoSlot1Candidate(Object* object,Adapter& a){a.serviceSlot10();auto* owner=a.owner14(object);a.setCounter29c(owner,a.counter29c(owner)-1u);}
