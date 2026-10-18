#include "controller_subsystem_cleanup_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Adapter{unsigned first=1,second=1,state=42;std::vector<unsigned> calls;
 auto initialized840(){return first;}auto initialized83c(){return second;}auto state844(){return &state;}
 void setInitialized840(unsigned n){first=n;}void setInitialized83c(unsigned n){second=n;}
 void cleanupA7b0(unsigned* p){assert(p==&state);calls.push_back(1);first=99;}
 void cleanup31100(){calls.push_back(2);second=0;}void cleanup30be0(){calls.push_back(3);}void cleanup304d0(){calls.push_back(4);second=99;}
};
int main(){Adapter a;cleanupControllerSubsystem2c90Candidate(a);cleanupControllerSubsystem27f0Candidate(a);assert((a.calls==std::vector<unsigned>{1,2,3,4}) && !a.first && !a.second);cleanupControllerSubsystem2c90Candidate(a);cleanupControllerSubsystem27f0Candidate(a);assert(a.calls.size()==4);std::cout<<"x64 subsystem cleanup: guards, helper order, callback state mutation and repeat no-op passed\n";}
