#pragma once
#include "element_filtered_callback_register_candidate.h"
#include "element_filtered_callback_clear_candidate.h"
#include "test_element_post_update_dispatch_fixture.h"
struct FilteredRegisterFixture {
    using Node=PostUpdateDispatchFixture::Filter;
    Node storage{};bool fail=false;
    unsigned packed=0,stateBits=0,queued=0;
    Node* allocateNode() {return fail?nullptr:&storage;}
    auto packedMask() {return packed;}
    auto state() {return stateBits;}
    void enqueue() {++queued;packed^=0x100;}
    void setPackedMask(unsigned p) {packed=p;}
};
inline void testElementFilteredCallbackRegisterFixture() {
    PostUpdateDispatchFixture::Callback cb{7};
    for(unsigned active=0;active<4;++active)
    for(unsigned previous=0;previous<4;++previous)
    for(unsigned mask=0;mask<8;++mask)
    for(unsigned enabled=0;enabled<2;++enabled)
    for(unsigned fail=0;fail<2;++fail) {
        FilteredRegisterFixture f;
        f.packed=0x80000000u|(previous<<2)|active;
        f.stateBits=enabled?0x10:0;f.fail=fail!=0;
        FilteredRegisterFixture::Node old{&cb,0,nullptr};auto* head=&old;
        registerElementFilteredCallbackCandidate(head,&cb,mask,f);
        const auto narrowed=previous&mask;
        const bool queued=enabled && (active&narrowed)==narrowed && (active&previous)!=previous;
        assert(f.queued==static_cast<unsigned>(queued));
        assert(f.packed==(0x80000000u|(queued?0x100u:0u)|(narrowed<<2)|active));
        if(fail)assert(head==&old);
        else assert(head==&f.storage && head->next==&old && head->callback==&cb && head->mask==mask);
    }
    PostUpdateDispatchFixture dispatch;
    using Node=PostUpdateDispatchFixture::Filter;
    auto* tail=new Node{&dispatch.always,0,nullptr};
    auto* middle=new Node{&dispatch.first,1,tail};
    auto* head=new Node{&dispatch.first,1,middle};
    dispatch.element.filters=head;
    const auto packed=dispatch.element.mask;
    unsigned freed=0;
    clearElementFilteredCallbacksCandidate(dispatch.element.filters,[&](Node* node) {
        assert(dispatch.element.filters==head);
        assert(node==(freed==0?head:freed==1?middle:tail));
        ++freed;node->next=nullptr;delete node;
    });
    assert(freed==3 && !dispatch.element.filters && dispatch.element.mask==packed);
    clearElementFilteredCallbacksCandidate(dispatch.element.filters,[&](Node*) {assert(false);});
    dispatch.dispatch();assert(dispatch.calls.empty());
}
