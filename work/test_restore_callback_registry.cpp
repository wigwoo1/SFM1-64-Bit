#include <cstdio>
#include <stdexcept>
#include <vector>
#include "native_manager_callback_registry_candidate.h"
struct Command{uintptr_t marker;};struct State{unsigned value;};
using Callback=void(*)(Command*,State*,State*,bool);
struct Allocator:NativeManagerCallbackAllocatorCandidate{
 bool fail=false;unsigned releases=0;
 void* reallocate(void* p,std::size_t n){return fail?nullptr:NativeManagerCallbackAllocatorCandidate::reallocate(p,n);}
 void release(void* p){if(p)++releases;NativeManagerCallbackAllocatorCandidate::release(p);}
};
using Registry=NativeManagerCallbackRegistryCandidateT<Allocator,Callback>;
static Registry* active;static bool appendDuringDispatch;static std::vector<unsigned> observed;
static Command* expectedCommand;static State* expectedCurrent;static State* expectedPrevious;
static bool expectedForce;
static void require(bool p){if(!p)throw std::runtime_error("restore registry mismatch");}
static void observe(Command* c,State* current,State* previous,bool force,unsigned id){
 require(c==expectedCommand&&current==expectedCurrent&&previous==expectedPrevious&&force==expectedForce);
 require(c->marker==uintptr_t{0x100001234ull});observed.push_back(id);++current->value;
}
static void second(Command* c,State* a,State* b,bool force){observe(c,a,b,force,2);}
static void first(Command* c,State* a,State* b,bool force){observe(c,a,b,force,1);if(appendDuringDispatch){appendDuringDispatch=false;active->append(second);}}
int main(){try{unsigned cases=0;
 for(bool force:{false,true}){
  Allocator allocator;{Registry registry{allocator};active=&registry;
   Command command{0x100001234ull};State current{0},previous{77};expectedCommand=&command;expectedCurrent=&current;expectedPrevious=&previous;expectedForce=force;
   require(reinterpret_cast<uintptr_t>(&command)>0xffffffffull&&reinterpret_cast<uintptr_t>(&current)>0xffffffffull&&reinterpret_cast<uintptr_t>(&previous)>0xffffffffull);
   for(unsigned i=0;i<8;++i)registry.append(first);
   allocator.fail=true;bool caught=false;try{registry.append(second);}catch(const std::bad_alloc&){caught=true;}
   require(caught&&registry.size()==8&&registry.capacity()==8);
   allocator.fail=false;appendDuringDispatch=true;observed.clear();registry.dispatchWith(&command,&current,&previous,force);
   require(observed==std::vector<unsigned>(8,1)&&registry.size()==9&&registry.capacity()==16&&current.value==8&&previous.value==77);
   observed.clear();registry.dispatchWith(&command,&current,&previous,force);auto expected=std::vector<unsigned>(8,1);expected.push_back(2);
   require(observed==expected&&current.value==17&&previous.value==77);
   require(registry.remove(second)&&registry.size()==8);++cases;
  }require(allocator.releases==1);
 }
 std::printf("restore_typed_callback_registry_cases=%u fullwidth_arguments_growth_and_failure=passed\n",cases);
 }catch(const std::exception& e){std::puts(e.what());return 1;}}
