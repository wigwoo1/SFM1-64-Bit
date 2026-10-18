#include "graph_bookmark_assignment_candidate.h"
#include "graph_bookmark_storage_composition_fixture.h"
#include <iostream>
struct Assignment {
 GraphBookmarkCopyFixture copy;int owner=0;bool allowed=true;unsigned dirty=0;
 int before=0,after=0;std::vector<int> events;
 auto* destinationBase(){return copy.storage.base0();}int destinationCount(){return copy.storage.count;}
 void remove103883e0(int,int){assert(false);}void insert103771c0(int,int){assert(false);}
 unsigned mutationGate103623b0(){events.push_back(1);return allowed;}
 unsigned recordOwnerModelSlot62(){events.push_back(2);return 0;}
 int* allocateNativeUndo(){assert(false);return nullptr;}
 int* construct1036c020(int*,std::uint32_t*,int,int){assert(false);return nullptr;}
 void submitModelSlot79(int*){assert(false);}
 auto* owner8(){return &owner;}unsigned flags11(){return 1;}
 unsigned ownerReady1035dc00(int* p){assert(p==&owner);return 1;}
 void ownerSlot2(int*,int start,int end){assert(start==0&&end==graphAliasDifference(before,1));events.push_back(3);}
 void copy1038a010(std::uint32_t* p,int n){events.push_back(4);graphBookmarkCopyCandidate(p,n,copy);}
 void ownerSlot1(int*,int start,int end){assert(start==0&&end==graphAliasDifference(after,1));assert(destinationCount()==after);events.push_back(5);}
 void ownerSlot0(int*){events.push_back(6);}unsigned type10(){return 8;}
 void mark1035eff0(bool references){assert(!references);events.push_back(7);}
 void orFrameworkFlags(unsigned f){dirty|=f;events.push_back(8);}
};
int main(){
 Assignment a;std::vector<std::uint32_t> input(1025);for(unsigned i=0;i<input.size();++i)input[i]=i*0x9e3779b9u;
 for(int pass=0;pass<2;++pass)for(int i=0;i<=1025;++i){
  const int n=pass?1025-i:i;a.before=a.destinationCount();a.after=n;a.events.clear();
  graphBookmarkAssignmentCandidate(input.data(),n,a);
  assert((a.events==std::vector<int>{1,2,3,4,5,6,7,8}));assert(a.dirty==0x4000);
  assert(std::vector<std::uint32_t>(a.copy.storage.data.begin(),a.copy.storage.data.begin()+n)==std::vector<std::uint32_t>(input.begin(),input.begin()+n));
 }
 const auto cap=a.copy.storage.cap;auto* base=a.destinationBase();a.allowed=false;a.events.clear();
 graphBookmarkAssignmentCandidate(input.data(),3,a);
 assert((a.events==std::vector<int>{1})&&a.destinationCount()==0&&a.copy.storage.cap==cap&&a.destinationBase()==base);
 std::cout<<"x64 attribute-to-storage bookmark composition: 2052 replacements, notification bounds/order and blocked mutation passed\n";
}
