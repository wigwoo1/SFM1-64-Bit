#pragma once
template<class Node,class Value,class Enqueue>
inline void prepareAttributeCallbackListCandidate(Value value,Node*& retained,
    Node*& dirty,bool suppressed,Enqueue&& enqueue) {
    if(suppressed)return;
    Node** link=&retained;
    while(*link) {
        auto* node=*link;
        if(node->value==value) {
            *link=node->next;node->next=dirty;dirty=node;
        } else link=&node->next;
    }
    enqueue();
}
