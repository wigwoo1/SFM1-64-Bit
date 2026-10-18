#include "controller_list_node_removal_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Node{int data;Node* previous;Node* next;};struct List{Node* head;Node* tail;std::uint32_t count;};
struct Adapter{std::vector<int> calls;List* list;bool mutate=false;
 void invokePayloadCallback(List& value,int payload){assert(&value==list && value.count==3);calls.push_back(payload);if(mutate)value.count=9;}
 void releaseNode(Node* node){assert(!calls.empty());calls.push_back(-node->data);}
};
int main(){for(int position=0;position<3;++position){Node n[3]={{1,nullptr,nullptr},{2,nullptr,nullptr},{3,nullptr,nullptr}};n[0].next=&n[1];n[1].previous=&n[0];n[1].next=&n[2];n[2].previous=&n[1];List list{&n[0],&n[2],3};Adapter a{{},&list};assert(removeControllerListNodeCandidate(list,&n[position],a));assert(list.count==2 && (a.calls==std::vector<int>{position+1,-(position+1)}));assert(list.head->previous==nullptr && list.tail->next==nullptr);if(position==1)assert(n[0].next==&n[2] && n[2].previous==&n[0]);}
 Node n{1,nullptr,nullptr};List list{&n,&n,3};Adapter a{{},&list,true};removeControllerListNodeCandidate(list,&n,a);assert(!list.head && !list.tail && list.count==8);
 List empty{nullptr,nullptr,0};Adapter b{{},&empty};assert(removeControllerListNodeCandidate(empty,&n,b) && b.calls.empty());assert(removeControllerListNodeCandidate(empty,static_cast<Node*>(nullptr),b));
 std::cout<<"x64 list removal: head/middle/tail, callback-before-count, live count mutation and no-op success passed\n";
}
