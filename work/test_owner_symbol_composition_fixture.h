#pragma once
#include "owner_symbol_hash_candidate.h"
#include "owner_symbol_installed_table_candidate.h"
#include "owner_symbol_chain_candidate.h"
#include "owner_symbol_lookup_protocol_candidate.h"
#include "owner_symbol_publication_protocol_candidate.h"
#include "owner_symbol_primary_publish_candidate.h"
#include "test_owner_symbol_arena_fixture.h"
#include <cassert>
#include <memory>
#include <string>
#include <vector>
// End-to-end, single-thread instrumentation fixture. This is not a production
// allocator, original pointer-token ABI, or a concurrent manager replacement.
struct OwnerSymbolCompositionFixture {
    using Base=OwnerSymbolChainNodeCandidate;
    using Key=OwnerSymbolKeyCandidate;
    struct Node:Base {
        Key ownedKey;
        std::uint32_t token;
        bool canonical=false;
        Node(const Key& source,std::uint32_t value):Base{nullptr,nullptr},
            ownedKey{source.hash,source.text},token(value) {
            key=&ownedKey;
        }
    };
    struct Bucket {Base* primary=nullptr;Base* pending=nullptr;int lock=0;};
    std::array<Bucket,2048> buckets;
    std::vector<std::unique_ptr<Node>> nodes;
    OwnerSymbolArenaFixture arena;
    bool changed=false;
    unsigned readLocks=0;
    Key makeKey(const char* text) {return {hashOwnerSymbolCandidate(text,ownerSymbolInstalledTableCandidate),text};}
    unsigned hash(const Key& key) {return key.hash;}
    Bucket* bucket(unsigned i) {return &buckets.at(i);}
    Base* root(Bucket* b) {return b->primary;}
    Base* secondary(Bucket* b) {return b->pending;}
    Base* context(Bucket* b) {return b->pending;}
    Base* search(const Key& key,Base* head,Base* stop) {
        return findOwnerSymbolChainCandidate(key,head,stop);
    }
    Base* search(const Key& key,Base* head,int stop) {
        assert(stop==0);return search(key,head,static_cast<Base*>(nullptr));
    }
    void enterRead(Bucket* b) {assert(b->lock==0);b->lock=1;++readLocks;}
    void leaveRead(Bucket* b) {assert(b->lock==1);b->lock=0;}
    void enter(Bucket* b) {enterRead(b);}
    void leave(Bucket* b) {leaveRead(b);}
    void enterWrite(Bucket* b) {assert(b->lock==0);b->lock=2;}
    void leaveWrite(Bucket* b) {assert(b->lock==2);b->lock=0;}
    Base* allocateLink(const Key& key,Bucket* b) {
        assert(b->lock==2);
        auto node=std::make_unique<Node>(key,static_cast<std::uint32_t>(nodes.size()));
        node->next=b->pending;
        auto* result=node.get();nodes.push_back(std::move(node));
        b->pending=result;changed=true;return result;
    }
    void storeKey(Base* base,const Key&) {static_cast<Node*>(base)->canonical=true;}
    std::uintptr_t symbol(Base* base) {auto* node=static_cast<Node*>(base);assert(node->canonical);return reinterpret_cast<std::uintptr_t>(node->key->text);}
    bool dirty() {return changed;}
    void clearDirty() {changed=false;}
    void setPrimary(Bucket* b,Base* head) {assert(b->lock==1);b->primary=head;}
    std::uintptr_t intern(const char* text) {
        const auto existing=lookup(text);
        if(existing!=std::numeric_limits<std::uintptr_t>::max() || !text)return existing;
        auto key=makeKey(text);key.text=arena.store(key.hash,text);
        return symbol(publishOwnerSymbolProtocolCandidate(key,*this));
    }
    std::uintptr_t lookup(const char* text) {return lookupOwnerSymbolProtocolCandidate(text,*this);}
};
inline void testOwnerSymbolCompositionFixture() {
    OwnerSymbolCompositionFixture table;
    assert(table.lookup(nullptr)==std::numeric_limits<std::uintptr_t>::max());
    assert(table.lookup("missing")==std::numeric_limits<std::uintptr_t>::max());
    const auto controls=table.intern("controls"),children=table.intern("children");
    assert(controls!=children && table.intern("controls")==controls);
    assert(table.lookup("Controls")==std::numeric_limits<std::uintptr_t>::max());
    assert(controls>0xffffffffull && children>0xffffffffull);
    const auto beforePending=table.readLocks;
    assert(table.lookup("controls")==controls && table.readLocks==beforePending+1);
    for (unsigned i=0;i<5000;++i) {
        const auto text=std::string("fixture_")+std::to_string(i);
        const auto token=table.intern(text.c_str());
        assert(table.lookup(text.c_str())==token);
    }
    assert(table.nodes.size()==5002);
    assert(table.arena.count>8 && table.arena.capacity>=table.arena.count);
    const auto firstAddress=reinterpret_cast<const char*>(controls);
    assert(std::strcmp(firstAddress,"controls")==0);
    const auto blockCount=table.arena.count;
    assert(table.intern("controls")==controls && table.arena.count==blockCount);
    publishOwnerSymbolPrimaryCandidate(table);
    const auto afterPublish=table.readLocks;
    assert(table.lookup("controls")==controls && table.lookup("children")==children);
    assert(table.readLocks==afterPublish);
    const auto added=table.intern("new_after_publish");
    assert(table.lookup("new_after_publish")==added);
    assert(table.intern("controls")==controls && table.nodes.size()==5003);
    publishOwnerSymbolPrimaryCandidate(table);
    const auto afterSecond=table.readLocks;
    assert(table.lookup("new_after_publish")==added && table.readLocks==afterSecond);
    for (unsigned i=0;i<5000;++i) {
        const auto text=std::string("fixture_")+std::to_string(i);
        assert(table.lookup(text.c_str())==reinterpret_cast<std::uintptr_t>(table.nodes[i+2]->key->text));
    }
    assert(table.readLocks==afterSecond);
    assert(reinterpret_cast<const char*>(controls)==firstAddress && std::strcmp(firstAddress,"controls")==0);
}
