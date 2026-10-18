#pragma once
#include <cstdint>
#include <limits>
//10359ca0 dispatch protocol; exact hash/key/tree/symbol storage remain bindings.
template<class Adapter>
std::uintptr_t lookupOwnerSymbolProtocolCandidate(const char* name,Adapter& adapter) {
    if (!name) return std::numeric_limits<std::uintptr_t>::max();
    auto key=adapter.makeKey(name);
    auto* bucket=adapter.bucket(adapter.hash(key)&0x7ffu);
    auto* node=adapter.search(key,adapter.root(bucket),0);
    if (!node) {
        adapter.enter(bucket);
        node=adapter.search(key,adapter.context(bucket),adapter.root(bucket));
        adapter.leave(bucket);
    }
    return node?adapter.symbol(node):std::numeric_limits<std::uintptr_t>::max();
}
