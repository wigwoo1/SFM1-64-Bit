#pragma once
#include "element_change_propagate_candidate.h"
#include "notification_suppression_flag_candidate.h"
#include "notification_suppression_scope_candidate.h"
#include <vector>
#include <cassert>
struct ElementChangeFixture {
    struct Element;
    struct Attribute {Element* owner;unsigned flags=0;};
    struct Reference {Attribute* attribute;Reference* next=nullptr;};
    struct Element {unsigned id,stateBits=0,packedBits=0;bool filtered=true;Reference* incoming=nullptr;};
    std::uint8_t suppressionField=0;
    std::vector<unsigned> resets,posts;
    bool suppressed() {return notificationSuppressionFlagCandidate(suppressionField);}
    std::uint8_t getSuppression() {return suppressed()?1:0;}
    void setSuppression(std::uint8_t v) {setNotificationSuppressionFlagCandidate(suppressionField,v);}
    auto state(Element* e) {return e->stateBits;}
    auto packed(Element* e) {return e->packedBits;}
    void setState(Element* e,unsigned s) {e->stateBits=s;}
    void setPacked(Element* e,unsigned p) {e->packedBits=p;}
    bool hasFilteredCallbacks(Element* e) {return e->filtered;}
    void enqueueReset(Element* e) {resets.push_back(e->id);}
    void enqueuePost(Element* e) {posts.push_back(e->id);}
    auto* firstIncoming(Element* e) {return e->incoming;}
    auto* nextIncoming(Reference* r) {return r->next;}
    auto* resolveAttribute(Reference* r) {return r->attribute;}
    auto attributeFlags(Attribute* p) {return p->flags;}
    auto* attributeOwner(Attribute* p) {return p->owner;}
};
inline void testElementChangePropagateFixture() {
    for(unsigned flags=0;flags<256;++flags)for(unsigned incoming=0;incoming<256;++incoming) {
        auto field=static_cast<std::uint8_t>(flags);
        setNotificationSuppressionFlagCandidate(field,static_cast<std::uint8_t>(incoming));
        assert((field&0xefu)==(flags&0xefu));
        assert(notificationSuppressionFlagCandidate(field)==bool(incoming&1u));
    }
    using F=ElementChangeFixture;
    F f;F::Element a{1},b{2},blocked{3};
    F::Attribute aa{&a},ab{&b},excluded{&blocked,0x1000},orphan{nullptr};
    F::Reference back{&aa},nullRef{nullptr},noOwner{&orphan,&nullRef},skip{&excluded,&noOwner},forward{&ab,&skip};
    a.incoming=&forward;b.incoming=&back;
    a.packedBits=b.packedBits=0x80000004u;
    setNotificationSuppressionFlagCandidate(f.suppressionField,1);propagateElementChangeCandidate(&a,3,f);
    assert(f.resets.empty() && a.stateBits==0);
    setNotificationSuppressionFlagCandidate(f.suppressionField,2);propagateElementChangeCandidate(&a,3,f);
    assert((f.resets==std::vector<unsigned>{1,2}) && (f.posts==std::vector<unsigned>{1,2}));
    assert(a.packedBits==0x80000007u && b.packedBits==0x80000005u && blocked.stateBits==0);
    propagateElementChangeCandidate(&a,3,f);
    assert(f.resets.size()==2 && f.posts.size()==2);
    F g;F::Element onlySecond{4},parent{5};F::Attribute pa{&parent};F::Reference pr{&pa};
    onlySecond.incoming=&pr;
    propagateElementChangeCandidate(&onlySecond,2,g);
    // Initial change propagates stripped zero mask to an unmarked parent.
    assert((g.resets==std::vector<unsigned>{4,5}) && parent.stateBits==0x10);
    assert(parent.packedBits==0 && onlySecond.packedBits==2);
    for(unsigned initial=0;initial<2;++initial) {
        F scoped;scoped.suppressionField=static_cast<std::uint8_t>(0xa5u|(initial<<4));
        F::Element changed{6};
        NotificationSuppressionScopeCandidate outer,inner;
        beginNotificationSuppressionScopeCandidate(outer,scoped);
        beginNotificationSuppressionScopeCandidate(inner,scoped);
        assert(outer.saved==initial && inner.saved==1 && scoped.suppressed());
        propagateElementChangeCandidate(&changed,1,scoped);assert(scoped.resets.empty());
        scoped.suppressionField^=0x80u; // Unrelated field mutation survives restoration.
        restoreNotificationSuppressionScopeCandidate(inner,scoped);
        assert(scoped.suppressed());
        restoreNotificationSuppressionScopeCandidate(outer,scoped);
        assert(scoped.suppressed()==bool(initial) && scoped.suppressionField==(0x25u|(initial<<4)));
        scoped.setSuppression(1);
        restoreNotificationSuppressionScopeCandidate(outer,scoped);
        assert(scoped.suppressed()); // Completed scope never restores twice.
    }
}
