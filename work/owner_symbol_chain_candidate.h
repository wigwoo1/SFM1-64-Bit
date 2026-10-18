#pragma once
#include <cstdint>
struct OwnerSymbolKeyCandidate {std::uint32_t hash;const char* text;};
struct OwnerSymbolChainNodeCandidate {
    const OwnerSymbolKeyCandidate* key;
    OwnerSymbolChainNodeCandidate* next;
};
inline int compareOwnerSymbolBytesCandidate(const char* a,const char* b) {
    while (true) {
        const auto x=static_cast<unsigned char>(*a++);
        const auto y=static_cast<unsigned char>(*b++);
        if (x!=y) return x<y?-1:1;
        if (!x) return 0;
    }
}
inline OwnerSymbolChainNodeCandidate* findOwnerSymbolChainCandidate(
    const OwnerSymbolKeyCandidate& key,OwnerSymbolChainNodeCandidate* node,
    OwnerSymbolChainNodeCandidate* stop) {
    for (;node!=stop;node=node->next)
        if (node->key->hash==key.hash &&
            compareOwnerSymbolBytesCandidate(node->key->text,key.text)==0) return node;
    return nullptr;
}
