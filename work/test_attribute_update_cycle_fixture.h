#pragma once
#include "test_element_post_update_dispatch_fixture.h"
#include "attribute_reference_mark_candidate.h"
#include "attribute_callback_prepare_candidate.h"
#include "element_state_mark_candidate.h"
#include "attribute_callback_register_candidate.h"
#include "attribute_callback_remove_candidate.h"
struct AttributeUpdateCycleFixture {
    using Framework=PostUpdateDispatchFixture;
    using Element=Framework::Element;
    Framework framework;
    struct Attribute {std::uint16_t flags=0x2000;Element* owner;Attribute* next=nullptr;} attribute{0x2000,&framework.element,nullptr};
    unsigned currentPhase=0,prepared=0;
    std::vector<unsigned> stateQueue,callbackQueue;
    unsigned allocated=0,freed=0;
    Framework::Node* allocateNode() {++allocated;return new Framework::Node{};}
    void freeNode(Framework::Node* p) {++freed;delete p;}
    void setCallbackFlag(Attribute* p) {p->flags|=0x2000;}
    void clearCallbackFlag(Attribute* p) {p->flags&=static_cast<std::uint16_t>(~0x2000u);}
    auto flags(Attribute* p) {return p->flags;}
    void setFlags(Attribute* p,std::uint16_t f) {p->flags=f;}
    auto phase() {return currentPhase;}
    auto* owner(Attribute* p) {return p->owner;}
    void prepare(Element* e,Attribute* p) {
        ++prepared;
        prepareAttributeCallbackListCandidate(static_cast<void*>(p),e->retained,e->dirty,false,
            [&](){callbackQueue.push_back(42);});
    }
    void markOwner(Element* e) {markElementStateCandidate(e,*this);}
    auto state(Element* e) {return e->state;}
    unsigned handle(Element*) {return 42;}
    void enqueue(unsigned h) {stateQueue.push_back(h);}
    void setState(Element* e,std::uint8_t s) {e->state=s;}
    Attribute* firstAttribute(Element*) {return &attribute;}
    auto* nextAttribute(Attribute* p) {return p->next;}
};
inline void testAttributeUpdateCycleFixture() {
    AttributeUpdateCycleFixture cycle;auto& f=cycle.framework;
    PostUpdateDispatchFixture::Node second{&f.future,&cycle.attribute,nullptr},first{&f.paired,&cycle.attribute,&second};
    f.element.retained=&first;
    assert(markAttributeReferenceCandidate(&cycle.attribute,cycle));
    assert(markAttributeReferenceCandidate(&cycle.attribute,cycle));
    assert(cycle.prepared==1 && cycle.stateQueue.size()==1 && cycle.callbackQueue.size()==1);
    assert(cycle.attribute.flags==0x6080 && f.element.dirty==&second && f.element.retained==nullptr);
    f.dispatch();
    assert((f.calls==std::vector<unsigned>{5,4}) && f.payloads.size()==2);
    assert(f.payloads[0]==&cycle.attribute && f.payloads[1]==&cycle.attribute);
    clearElementAttributeMarkCandidate(&f.element,cycle);
    assert(cycle.attribute.flags==0x6000 && !(f.element.state&1u));
    cycle.currentPhase=5;
    assert(!markAttributeReferenceCandidate(&cycle.attribute,cycle));
    assert(cycle.prepared==1 && cycle.stateQueue.size()==1);
    cycle.currentPhase=0;
    assert(markAttributeReferenceCandidate(&cycle.attribute,cycle));
    assert(cycle.prepared==2 && cycle.stateQueue.size()==2 && cycle.callbackQueue.size()==2);
    f.calls.clear();f.payloads.clear();f.dispatch();
    assert((f.calls==std::vector<unsigned>{4,5}) && f.payloads[0]==&cycle.attribute);
    assert(f.element.dirty==nullptr && f.element.retained==&first);
}
inline void testAttributeSubscriptionLifecycleFixture() {
    AttributeUpdateCycleFixture cycle;auto& f=cycle.framework;
    cycle.attribute.flags=0;
    auto subscribe=[&](auto* callback) {
        registerAttributeCallbackCandidate(f.element.retained,callback,&cycle.attribute,cycle);
    };
    auto unsubscribe=[&](auto* callback) {
        removeAttributeCallbackCandidate(f.element.retained,f.element.dirty,callback,&cycle.attribute,cycle);
    };
    subscribe(&f.paired);subscribe(&f.future);subscribe(&f.paired);
    assert(cycle.allocated==3 && cycle.attribute.flags==0x2000);
    assert(markAttributeReferenceCandidate(&cycle.attribute,cycle));
    assert(f.element.retained==nullptr && f.element.dirty);
    // Unsubscribe while queued: remove one duplicate from the dirty chain.
    unsubscribe(&f.paired);
    assert(cycle.freed==1 && cycle.attribute.flags==0x6080);
    f.dispatch();
    assert((f.calls==std::vector<unsigned>{5,4}));
    assert(f.payloads.size()==2 && f.payloads[0]==&cycle.attribute && f.payloads[1]==&cycle.attribute);
    clearElementAttributeMarkCandidate(&f.element,cycle);
    unsubscribe(&f.future);
    assert(cycle.freed==2 && cycle.attribute.flags==0x6000);
    assert(markAttributeReferenceCandidate(&cycle.attribute,cycle));
    f.calls.clear();f.payloads.clear();f.dispatch();
    assert((f.calls==std::vector<unsigned>{4}) && f.payloads[0]==&cycle.attribute);
    unsubscribe(&f.paired);
    assert(cycle.freed==cycle.allocated && cycle.attribute.flags==0x4080);
    assert(!f.element.retained && !f.element.dirty);
    clearElementAttributeMarkCandidate(&f.element,cycle);
    const auto prepared=cycle.prepared;
    assert(markAttributeReferenceCandidate(&cycle.attribute,cycle));
    assert(cycle.prepared==prepared); // Last removal disables callback preparation.
    f.calls.clear();f.dispatch();assert(f.calls.empty());
    unsubscribe(&f.paired);assert(cycle.freed==3);
}
