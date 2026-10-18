#pragma once
template<class Node,class Value>
inline bool hasAttributeCallbackCandidate(Node* head,Value value) {
    for(auto* node=head;node;node=node->next)if(node->value==value)return true;
    return false;
}
template<class Node,class Callback,class Value,class Free>
inline bool removeAttributeCallbackPairCandidate(Node*& head,Callback callback,Value value,Free&& freeNode) {
    Node** link=&head;
    while(*link) {
        auto* node=*link;
        if(node->callback==callback && node->value==value) {
            *link=node->next;freeNode(node);return true;
        }
        link=&node->next;
    }
    return false;
}
template<class Node,class Callback,class Attribute,class Adapter>
inline void removeAttributeCallbackCandidate(Node*& retained,Node*& dirty,
    Callback callback,Attribute* attribute,Adapter& a) {
    if(!attribute)return;
    auto freeNode=[&](Node* node){a.freeNode(node);};
    if(!removeAttributeCallbackPairCandidate(retained,callback,attribute,freeNode) &&
       !removeAttributeCallbackPairCandidate(dirty,callback,attribute,freeNode))return;
    if(!hasAttributeCallbackCandidate(retained,attribute) && !hasAttributeCallbackCandidate(dirty,attribute))
        a.clearCallbackFlag(attribute);
}
