#include "controller_subsystem_initialization_candidate.h"
#include "controller_shutdown_tail_candidate.h"
#include "controller_startup_guard_candidate.h"
#include "controller_subsystem_cleanup_candidate.h"
#include "controller_startup_generator_candidate.h"
#include "controller_startup_seed_candidate.h"
#include <cassert>
#include <vector>
#include <string>
#include <iostream>
struct Adapter{
 unsigned refs=0,flags=0,guard=0,collection=0,state=0;bool startup=true;unsigned socketResult=0;std::vector<std::string> events;
 unsigned referenceCount(){return refs;}void setReferenceCount(unsigned n){refs=n;}
 unsigned subsystemFlags(){return flags;}void setSubsystemFlags(unsigned n){flags=n;}
 unsigned initialized83c(){return guard;}void setInitialized83c(unsigned n){guard=n;}
 unsigned initialized840(){return collection;}void setInitialized840(unsigned n){collection=n;}unsigned* state844(){return &collection;}
 void installAllocatorHooks(){events.push_back("hooks");}
 bool initialize27d0(){return initializeControllerStartupGuardCandidate(*this);}
 void initialize30180(){events.push_back("30180");}bool initialize30020(){events.push_back("30020");return startup;}
 void initialize30010(){events.push_back("30010");}void initialize30000(){events.push_back("30000");}
 unsigned initialize0d60(){events.push_back("socket-init");return socketResult;}
 void initialize25e0(){events.push_back("seed");seedControllerStartupRandomCandidate(*this);}
 unsigned randomState(){return state;}void setRandomState(unsigned n){state=n;}
 unsigned seedHelper(unsigned n){assert(!n);return static_cast<unsigned>(controllerStartupSeedCandidate(116444736010000000ULL));}
 void cleanup2c90(){cleanupControllerSubsystem2c90Candidate(*this);}
 void cleanup27f0(){cleanupControllerSubsystem27f0Candidate(*this);}
 void cleanupA7b0(unsigned*){events.push_back("collection-release");}
 void cleanup31100(){events.push_back("31100");}void cleanup30be0(){events.push_back("30be0");}void cleanup304d0(){events.push_back("304d0");}
 void cleanup0d50(){events.push_back("socket-cleanup");}
};
int main(){Adapter a;assert(initializeControllerSubsystemCandidate(3,a)==0 && a.refs==1 && a.flags==3 && a.guard==1 && a.state!=0);const auto state=a.state;auto before=a.events.size();assert(initializeControllerSubsystemCandidate(0,a)==0 && a.refs==2 && a.events.size()==before && a.state==state);controllerSubsystemReleaseCandidate(a);assert(a.refs==1 && a.events.size()==before);controllerSubsystemReleaseCandidate(a);assert(!a.refs && !a.flags && !a.guard);assert((std::vector<std::string>(a.events.begin()+before,a.events.end())==std::vector<std::string>{"31100","30be0","304d0","socket-cleanup"}));
 Adapter b;b.startup=false;assert(initializeControllerSubsystemCandidate(3,b)==2 && b.refs==1 && b.guard==1 && !b.flags && !b.state);controllerSubsystemReleaseCandidate(b);assert(!b.refs && b.guard==1);b.startup=true;before=b.events.size();assert(initializeControllerSubsystemCandidate(1,b)==0 && b.events.size()==before+2 && b.guard==1);controllerSubsystemReleaseCandidate(b);assert(!b.guard);
 Adapter c;c.socketResult=1;assert(initializeControllerSubsystemCandidate(3,c)==2 && c.refs==1 && c.guard==1 && !c.flags);controllerSubsystemReleaseCandidate(c);assert(c.guard==1 && !c.refs);
 std::cout<<"x64 composed lifecycle: seeded startup, nested references, shutdown order and retained state after initialization failures passed\n";
}
