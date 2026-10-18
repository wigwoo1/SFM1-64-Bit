#include "controller_startup_guard_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Adapter{unsigned state=0;bool success=true;std::vector<unsigned> calls;
 unsigned initialized83c(){return state;}void setInitialized83c(unsigned n){state=n;}
 void initialize30180(){assert(state==1);calls.push_back(1);}bool initialize30020(){calls.push_back(2);return success;}
 void initialize30010(){calls.push_back(3);}void initialize30000(){calls.push_back(4);}
};
int main(){Adapter a;assert(initializeControllerStartupGuardCandidate(a) && (a.calls==std::vector<unsigned>{1,2,3,4}));assert(initializeControllerStartupGuardCandidate(a) && a.calls.size()==4);
 Adapter b;b.success=false;assert(!initializeControllerStartupGuardCandidate(b) && b.state==1 && b.calls.size()==2);assert(initializeControllerStartupGuardCandidate(b) && b.calls.size()==2);
 std::cout<<"x64 startup guard: ordered success, failed initialization retains flag and subsequent cached success passed\n";
}
