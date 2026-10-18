#pragma once
// Portable103604ae..1036069c normal linked-list path. Node layout and payload
// interpretation belong to caller; original callback0/value4/next8.
template<class Node,class AppendCallback,class AppendValue>
inline void collectRotateElementCallbackListCandidate(Node*& dirty,Node*& retained,
    AppendCallback&& appendCallback,AppendValue&& appendValue) {
    Node** link=&dirty;
    while(*link) {
        auto* node=*link;
        appendCallback(node->callback);
        appendValue(node->value);
        link=&node->next;
    }
    *link=retained;retained=nullptr;
    auto* previous=retained;retained=dirty;dirty=previous;
}
