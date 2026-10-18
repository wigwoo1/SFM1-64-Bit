#include "controller_collection_disposal_candidate.h"
#include "controller_list_node_removal_candidate.h"
#include <cassert>
#include <cstdlib>
#include <unordered_set>
#include <vector>
#include <iostream>
struct Node{int data;Node* previous;Node* next;};struct List{Node* head=nullptr;Node* tail=nullptr;unsigned count=0;};
struct Adapter{
 List** array;unsigned count=4;std::unordered_set<void*> live;std::vector<int> payloads;unsigned nodesReleased=0,listsReleased=0,arraysReleased=0;
 Adapter(){array=static_cast<List**>(std::calloc(count,sizeof(List*)));live.insert(array);}
 void addList(unsigned index,unsigned nodes){auto* list=static_cast<List*>(std::calloc(1,sizeof(List)));live.insert(list);array[index]=list;for(unsigned i=0;i<nodes;++i){auto* node=static_cast<Node*>(std::calloc(1,sizeof(Node)));live.insert(node);node->data=static_cast<int>(index*100+i);node->previous=list->tail;if(list->tail)list->tail->next=node;else list->head=node;list->tail=node;++list->count;}}
 unsigned collectionCount(){return count;}List* collectionItem(unsigned i){return array[i];}void clearCollectionItem(unsigned i){array[i]=nullptr;}List** collectionStorage(){return array;}
 unsigned itemCount(List* p){return p->count;}Node* itemField4(List* p){return p->tail;}
 void removeItem3f80(List* list,Node* node){assert(removeControllerListNodeCandidate(*list,node,*this));}
 void invokePayloadCallback(List& list,int payload){assert(list.count>0);payloads.push_back(payload);if(list.tail)assert(!list.tail->next);else assert(!list.head);}
 void releaseNode(Node* node){assert(live.erase(node)==1);++nodesReleased;std::free(node);}
 void releasePointer(List* list){assert(!list->head && !list->tail && !list->count && live.erase(list)==1);++listsReleased;std::free(list);}
 void releasePointer(List** storage){assert(storage==array);for(unsigned i=0;i<count;++i)assert(!storage[i]);assert(live.erase(storage)==1);++arraysReleased;std::free(storage);}
};
int main(){static_assert(sizeof(void*)==8);unsigned cases=0;for(unsigned nodes:{0u,1u,2u,31u,256u}){Adapter a;a.addList(0,nodes);a.addList(2,nodes);a.addList(3,0);disposeControllerCollectionCandidate(a);assert(a.live.empty() && a.nodesReleased==2*nodes && a.listsReleased==3 && a.arraysReleased==1 && a.count==4);for(unsigned i=0;i<nodes;++i){assert(a.payloads[i]==static_cast<int>(nodes-1-i));assert(a.payloads[nodes+i]==static_cast<int>(200+nodes-1-i));}++cases;}
 std::cout<<"native x64 collection disposal: "<<cases<<" sizes, real allocations, reverse node order, null/empty lists and exactly-once release passed\n";
}
