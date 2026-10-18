#include "graph_bookmark_undo_methods_candidate.h"
#include <cstdint>
#include <vector>
#include <cassert>
#include <iostream>
struct Adapter{
 struct Vector{int count=0;};Vector original,replacement;Vector* current=&original;
 int owner=0,attribute=0;unsigned type=21,clears=0,inserts=0,finishes=0;bool missing=false,finish=false,mutate=false;
 std::vector<unsigned> old{1,0x80000000,3},next{4,5};std::vector<unsigned> restored;
 std::uintptr_t savedName14(){return ~std::uintptr_t(0);}bool invalidName(std::uintptr_t n){return n==~std::uintptr_t(0);}
 std::uintptr_t defaultName10a8a4f0(){return 0x100000042ull;}
 unsigned savedHandle18(){return 9;}int* resolveModelSlot18(unsigned h){assert(h==9);return missing?nullptr:&owner;}
 int* find1035dd50(int*,std::uintptr_t n){assert(n==0x100000042ull);return &attribute;}
 unsigned type10(int*){return type;}auto* data4(int*){return current;}
 void clear10377310(int*,Vector* v){assert(v==&original);++clears;original.count=0;restored.clear();if(mutate)current=&replacement;}
 int snapshotCount(bool redo){return int((redo?next:old).size());}auto* snapshotBase(bool redo){return (redo?next:old).data();}
 int count0c(Vector* v){assert(v==&original);return v->count;}
 void insert103770a0(int*,Vector* v,int index,unsigned* value){assert(v==current&&index==original.count);++inserts;restored.push_back(*value);++original.count;}
 bool flag2c(){return finish;}void finish10377420(int*,Vector* v){assert(v==current);++finishes;}
};
int main(){Adapter a;a.mutate=true;graphBookmarkUndoReplayCandidate(false,a);assert(a.restored==a.old&&a.inserts==3&&!a.finishes);
 Adapter b;b.finish=true;graphBookmarkUndoReplayCandidate(true,b);assert(b.restored==b.next&&b.finishes==1);
 Adapter c;c.next.clear();c.finish=true;graphBookmarkUndoReplayCandidate(true,c);assert(c.clears==1&&!c.inserts&&c.finishes==1);
 Adapter d;d.missing=true;graphBookmarkUndoReplayCandidate(false,d);assert(!d.clears);
 Adapter e;e.type=8;graphBookmarkUndoReplayCandidate(true,e);assert(!e.clears);
 std::cout<<"x64 bookmark undo/redo protocol: snapshots, captured versus live vector, empty redo completion and lookup/type gates passed\n";
}
