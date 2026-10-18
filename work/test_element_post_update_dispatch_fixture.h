#pragma once
#include "element_post_update_dispatch_candidate.h"
#include "element_callback_storage_candidate.h"
#include <vector>
#include <cstdlib>
struct PostUpdateDispatchFixture {
    struct Callback {unsigned id;};
    struct Node {Callback* callback;void* value;Node* next;};
    struct Filter {Callback* callback;unsigned mask;Filter* next;};
    struct Element {unsigned state=0x10,mask=1;Filter* filters=nullptr;Node* dirty=nullptr;Node* retained=nullptr;} element;
    Callback first{1},always{2},excluded{3},paired{4},future{5};
    int payload=0,futurePayload=0;
    Node futureNode{&future,&futurePayload,nullptr};
    std::vector<unsigned> calls,masks;
    std::vector<void*> payloads;
    auto state(Element* e) {return e->state;}
    auto currentMask(Element* e) {return e->mask;}
    auto* filteredHead(Element* e) {return e->filters;}
    auto* filteredNext(Filter* f) {return f->next;}
    auto requiredMask(Filter* f) {return f->mask;}
    auto* filteredCallback(Filter* f) {return f->callback;}
    Node*& dirtyHead(Element* e) {return e->dirty;}
    Node*& retainedHead(Element* e) {return e->retained;}
    void invokeFiltered(Callback* c,Element* e,unsigned mask) {
        calls.push_back(c->id);masks.push_back(mask);
        if(c==&first) {e->mask=2;e->dirty=&futureNode;}
    }
    void invokeValue(Callback* c,void* p) {calls.push_back(c->id);payloads.push_back(p);}
    template<class Pointer> struct Allocator {
        Pointer* allocate(std::size_t n) {auto p=static_cast<Pointer*>(std::malloc(n));assert(p);return p;}
        Pointer* reallocate(Pointer* p,std::size_t n) {auto q=static_cast<Pointer*>(std::realloc(p,n));assert(q);return q;}
        void free(Pointer* p) {std::free(p);}
    };
    void dispatch() {
        Allocator<Callback*> ca;Allocator<void*> va;
        ElementCallbackStorageCandidate<Callback*,Allocator<Callback*>> filtered(ca),callbacks(ca);
        ElementCallbackStorageCandidate<void*,Allocator<void*>> values(va);
        dispatchElementPostUpdateCandidate(&element,*this,filtered,callbacks,values);
    }
};
inline void testElementPostUpdateDispatchFixture() {
    PostUpdateDispatchFixture f;
    PostUpdateDispatchFixture::Filter zero{&f.always,0,nullptr},skip{&f.excluded,2,&zero},one{&f.first,1,&skip};
    PostUpdateDispatchFixture::Node retained{&f.excluded,nullptr,nullptr},b{&f.paired,&f.payload,nullptr},a{&f.paired,&f.payload,&b};
    f.element.filters=&one;f.element.dirty=&a;f.element.retained=&retained;
    f.dispatch();
    assert((f.calls==std::vector<unsigned>{1,2,4,4}) && (f.masks==std::vector<unsigned>{1,2}));
    assert(f.payloads.size()==2 && f.payloads[0]==&f.payload && f.payloads[1]==&f.payload);
    assert(f.element.dirty==&f.futureNode && f.element.retained==&a && b.next==&retained);
    f.calls.clear();f.masks.clear();f.payloads.clear();f.element.state=0;
    f.dispatch();
    assert((f.calls==std::vector<unsigned>{5}) && f.payloads[0]==&f.futurePayload);
    assert(f.element.dirty==nullptr && f.element.retained==&f.futureNode && f.futureNode.next==&a);
    f.calls.clear();f.dispatch();assert(f.calls.empty());
}
