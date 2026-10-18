#pragma once
#include "test_element_post_update_dispatch_fixture.h"
#include "element_filtered_callback_register_candidate.h"
#include "element_filtered_callback_clear_candidate.h"
#include "element_change_propagate_candidate.h"
#include "element_change_reset_candidate.h"
#include "element_framework_post_queues_candidate.h"
#include "element_notification_initialize_candidate.h"
struct FilteredChangeCycleFixture:PostUpdateDispatchFixture {
    struct Attribute {Element* owner;};
    struct Reference {Attribute* attribute;Reference* next;};
    std::vector<unsigned> posts,resets;
    unsigned registered=0,freed=0;
    bool suppressed() {return false;}
    auto packed(Element* e) {return e->mask;}
    void setPacked(Element* e,unsigned p) {e->mask=p;}
    void setState(Element* e,unsigned s) {e->state=s;}
    void clearNotificationHeads(Element* e) {e->filters=nullptr;e->dirty=nullptr;e->retained=nullptr;}
    bool hasFilteredCallbacks(Element* e) {return e->filters!=nullptr;}
    void enqueueReset(Element*) {resets.push_back(42);}
    void enqueuePost(Element*) {posts.push_back(42);}
    Reference* firstIncoming(Element*) {return nullptr;}
    Reference* nextIncoming(Reference* r) {return r->next;}
    Attribute* resolveAttribute(Reference* r) {return r->attribute;}
    unsigned attributeFlags(Attribute*) {return 0;}
    Element* attributeOwner(Attribute* p) {return p->owner;}
    struct Registration {
        FilteredChangeCycleFixture& f;
        Filter* allocateNode() {++f.registered;return new Filter{};}
        auto packedMask() {return f.element.mask;}
        auto state() {return f.element.state;}
        void enqueue() {f.enqueuePost(&f.element);}
        void setPackedMask(unsigned p) {f.element.mask=p;}
    };
    auto postCount() {return static_cast<int>(posts.size());}
    auto* postData() {return posts.data();}
    void clearPostCount() {posts.clear();}
    int callbackCount() {return 0;}
    unsigned* callbackData() {return nullptr;}
    void invoke(unsigned) {assert(false);}
    auto resetCount() {return static_cast<int>(resets.size());}
    auto* resetData() {return resets.data();}
    void clearResetCount() {resets.clear();}
    Element* resolve(unsigned h) {return h==42?&element:nullptr;}
    void postUpdate(Element*) {dispatch();}
    void reset(Element* e) {resetElementChangeCandidate(e,*this);}
};
inline void testFilteredChangeCycleFixture() {
    FilteredChangeCycleFixture f;
    f.element.state=0x3f;f.element.mask=0x8000000fu;
    initializeElementNotificationCandidate(&f.element,f);
    assert(f.element.state==0x20 && f.element.mask==0x80000000u);
    FilteredChangeCycleFixture::Registration registration{f};
    registerElementFilteredCallbackCandidate(f.element.filters,&f.always,1u,registration);
    assert(f.element.mask==0x80000000u && f.registered==1);
    propagateElementChangeCandidate(&f.element,2,f);
    // Required aggregate zero queues immediately even if callback mask1
    // is not yet satisfied; dispatch filters the callback independently.
    assert(f.posts.size()==1 && f.resets.size()==1);
    propagateElementChangeCandidate(&f.element,1,f);
    assert(f.posts.size()==1 && f.resets.size()==1);
    processElementFrameworkPostQueuesCandidate(f);
    assert((f.calls==std::vector<unsigned>{2}) && (f.masks==std::vector<unsigned>{3}));
    assert(f.element.mask==0x80000000u && !(f.element.state&0x10u));
    f.calls.clear();f.masks.clear();
    propagateElementChangeCandidate(&f.element,1,f);
    processElementFrameworkPostQueuesCandidate(f);
    assert((f.calls==std::vector<unsigned>{2}) && (f.masks==std::vector<unsigned>{1}));
    clearElementFilteredCallbacksCandidate(f.element.filters,[&](auto* p){++f.freed;delete p;});
    assert(f.freed==f.registered);
    f.calls.clear();propagateElementChangeCandidate(&f.element,1,f);
    processElementFrameworkPostQueuesCandidate(f);assert(f.calls.empty());
    FilteredChangeCycleFixture separate;
    initializeElementNotificationCandidate(&separate.element,separate);
    FilteredChangeCycleFixture::Registration separateRegistration{separate};
    registerElementFilteredCallbackCandidate(separate.element.filters,&separate.always,1u,separateRegistration);
    propagateElementChangeCandidate(&separate.element,2,separate);
    assert(separate.posts.size()==1);
    processElementFrameworkPostQueuesCandidate(separate);
    assert(separate.calls.empty() && (separate.element.mask&3u)==0);
    propagateElementChangeCandidate(&separate.element,1,separate);
    processElementFrameworkPostQueuesCandidate(separate);
    assert((separate.calls==std::vector<unsigned>{2}));
    clearElementFilteredCallbacksCandidate(separate.element.filters,[&](auto* p){++separate.freed;delete p;});
    assert(separate.freed==separate.registered);
}
