#include "graph_state_finalization_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Adapter{unsigned state=1,flag=1;int first=1,second=2,object=3;int* owner=&first;bool fail=false;std::vector<int> calls;
 void setByte9(unsigned n){flag=n;calls.push_back(1);}unsigned state10(){assert(!flag);return state;}
 void beforeUndo100c09b0(){calls.push_back(2);owner=&second;}
 int* allocateNativeUndo(){calls.push_back(3);return fail?nullptr:&object;}int* owner4(){return owner;}
 int* construct100bf820(int* p,int* o){assert(p==&object && o==&second);calls.push_back(4);return p;}
 void submitModelSlot79(int* p){assert(p==(fail?nullptr:&object));calls.push_back(5);state=99;}
 void endModelSlot67(){calls.push_back(6);}void setState10(unsigned n){state=n;calls.push_back(7);}
};
int main(){Adapter a;graphStateFinalizationCandidate(a);assert((a.calls==std::vector<int>{1,2,3,4,5,6,7}) && !a.state);Adapter b;b.fail=true;graphStateFinalizationCandidate(b);assert((b.calls==std::vector<int>{1,2,3,5,6,7}) && !b.state);Adapter c;c.state=0;graphStateFinalizationCandidate(c);assert((c.calls==std::vector<int>{1,7}));std::cout<<"x64 state finalization: flag/state gates, live owner, null undo submission and callback state clearing passed\n";}
