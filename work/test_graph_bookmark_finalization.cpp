#include "graph_state_finalization_candidate.h"
#include "graph_undo_constructor_candidate.h"
#include "graph_undo_methods_candidate.h"
#include <vector>
#include <cassert>
#include <iostream>
struct Owner{unsigned counter=0;};
struct FinalizeAdapter{
 int state=1;unsigned char byte9=1;Owner owner;GraphUndoObjectCandidate* object=nullptr;
 unsigned allocations=0;std::vector<int> events;
 void setByte9(unsigned char n){byte9=n;events.push_back(1);}
 int state10(){return state;}void setState10(int n){state=n;events.push_back(9);}
 void beforeUndo100c09b0(){events.push_back(2);}
 auto* allocateNativeUndo(){events.push_back(3);++allocations;return new GraphUndoObjectCandidate;}
 auto* owner4(){return &owner;}
 GraphUndoObjectCandidate* construct100bf820(GraphUndoObjectCandidate* p,Owner* o){
  events.push_back(4);struct Capture{int n=0;unsigned captureModelSlot80(const char*){return ++n;}
   unsigned captureModelSlot81(const char*){return ++n;}} capture;
  return constructGraphUndoCandidate(p,o,"Stop Ignoring Log Changes",capture);
 }
 void submitModelSlot79(GraphUndoObjectCandidate* p){events.push_back(5);object=p;}
 void endModelSlot67(){events.push_back(6);}
 auto* owner14(GraphUndoObjectCandidate* p){return static_cast<Owner*>(p->owner14);}
 unsigned counter29c(Owner* o){return o->counter;}void setCounter29c(Owner* o,unsigned n){o->counter=n;}
 void serviceSlot10(){events.push_back(7);}
};
int main(){
 FinalizeAdapter a;graphStateFinalizationCandidate(a);
 assert(a.object&&a.state==0&&!a.byte9&&a.object->owner14==&a.owner);
 assert(std::string(a.object->label)=="Stop Ignoring Log Changes");
 assert((a.events==std::vector<int>{1,2,3,4,5,6,9}));
 graphUndoSlot0Candidate(a.object,a);assert(a.owner.counter==1);
 graphUndoSlot1Candidate(a.object,a);assert(a.owner.counter==0);
 delete a.object;FinalizeAdapter b;b.state=0;graphStateFinalizationCandidate(b);
 assert(!b.allocations&&(b.events==std::vector<int>{1,9}));
 std::cout<<"x64 bookmark edit finalization: boundary record construction/order, state clearing and undo/redo owner counter passed\n";
}
