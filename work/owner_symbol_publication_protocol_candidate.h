#pragma once
#include <cstdint>
//10357410 protocol: read retry and write retry search secondary->primary.
// Allocation/link/key store remain original manager bindings. Callbacks must
// not throw while locks are held, matching original service preconditions.
template<class Key,class Adapter>
auto publishOwnerSymbolProtocolCandidate(const Key& key,Adapter& adapter) {
    auto* bucket=adapter.bucket(adapter.hash(key)&0x7ffu);
    auto* node=adapter.search(key,adapter.root(bucket),0);
    if (node) return node;
    adapter.enterRead(bucket);
    node=adapter.search(key,adapter.secondary(bucket),adapter.root(bucket));
    adapter.leaveRead(bucket);
    if (node) return node;
    adapter.enterWrite(bucket);
    node=adapter.search(key,adapter.secondary(bucket),adapter.root(bucket));
    if (!node) {
        node=adapter.allocateLink(key,bucket);
        adapter.storeKey(node,key);
    }
    adapter.leaveWrite(bucket);
    return node;
}
