#pragma once
// 1035e640 frees every node using the filtered-node pool and only then
// clears the head. Does not recompute element packed flags or free callbacks.
template<class Node,class Free>
inline void clearElementFilteredCallbacksCandidate(Node*& head,Free&& freeNode) {
    auto* node=head;
    while(node) {
        auto* next=node->next;
        freeNode(node);
        node=next;
    }
    head=nullptr;
}
