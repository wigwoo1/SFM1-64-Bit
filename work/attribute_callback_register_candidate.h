#pragma once
template<class Node,class Callback,class Attribute,class Adapter>
inline void registerAttributeCallbackCandidate(Node*& retained,
    Callback callback,Attribute* attribute,Adapter& a) {
    if(!attribute)return;
    auto* node=a.allocateNode();
    if(node) {
        node->callback=callback;node->value=attribute;node->next=nullptr;
        node->next=retained;retained=node;
    }
    a.setCallbackFlag(attribute);
}
