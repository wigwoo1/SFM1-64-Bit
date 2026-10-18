#pragma once
#include <cstdint>
template<class List,class Node,class Adapter>
bool removeControllerListNodeCandidate(List& list,Node* node,Adapter& a){
 if(!node || !list.count)return true;
 if(node==list.head){list.head=node->next;if(node->next)node->next->previous=nullptr;else list.tail=nullptr;}
 else{node->previous->next=node->next;if(node->next)node->next->previous=node->previous;else list.tail=node->previous;}
 a.invokePayloadCallback(list,node->data);
 a.releaseNode(node);
 --list.count;
 return true;
}
