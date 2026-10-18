#pragma once
//10354440 post-allocation protocol; adapter supplies real allocator/locking.
// Caller already holds bucket write lock. Allocation must return a valid node.
template<class Key,class Bucket,class Adapter>
auto linkOwnerSymbolNodeCandidate(const Key& key,Bucket* bucket,Adapter& adapter) {
    adapter.markDirty();
    adapter.enterAllocator();
    auto* node=adapter.allocateNode();
    adapter.leaveAllocator();
    adapter.setNext(node,adapter.secondary(bucket));
    adapter.setSecondary(bucket,node);
    adapter.setSearchKey(node,key);
    return node;
}
