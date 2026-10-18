#include "controller_collection_disposal_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Item{unsigned count;int field;};
struct Adapter{
 Item first{2,10},second{1,20};Item* original[3]={&first,nullptr,&second};Item* changed[3]={&first,nullptr,&second};Item** array=original;unsigned count=3;bool mutate=false;std::vector<void*> released;std::vector<int> removed;
 unsigned collectionCount(){return count;}Item* collectionItem(unsigned i){return array[i];}void clearCollectionItem(unsigned i){array[i]=nullptr;}Item** collectionStorage(){return array;}
 unsigned itemCount(Item* p){return p->count;}int itemField4(Item* p){return p->field;}
 void removeItem3f80(Item* p,int field){removed.push_back(field);--p->count;++p->field;if(mutate){array=changed;count=1;}}
 void releasePointer(void* p){released.push_back(p);}
};
int main(){Adapter a;disposeControllerCollectionCandidate(a);assert((a.removed==std::vector<int>{10,11,20}) && a.released.size()==3 && a.released.back()==a.original && a.count==3 && !a.original[0] && !a.original[2]);
 Adapter b;b.mutate=true;disposeControllerCollectionCandidate(b);assert(b.released.size()==2 && b.released.back()==b.changed && !b.changed[0] && b.original[0]==&b.first && b.second.count==1);
 Adapter c;c.count=0xffffffff;disposeControllerCollectionCandidate(c);assert(c.removed.empty() && c.released.size()==1);
 std::cout<<"x64 collection disposal: null slots, live item fields/count, live array/count replacement and signed outer gate passed\n";
}
