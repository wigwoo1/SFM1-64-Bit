#include "owner_symbol_pointer_growth_candidate.h"
#include "element_owner_entry_insert_candidate.h"
#include "element_owner_bind_candidate.h"
#include "element_owner_release_candidate.h"
#include "element_owner_recycle_queue_candidate.h"
#include "test_element_owner_lifecycle_fixture.h"
#include "element_owner_destroy_protocol_candidate.h"
#include "element_cleanup_release_decision_candidate.h"
#include "attribute_reference_mark_candidate.h"
#include "element_state_mark_candidate.h"
#include "element_state_queue_append_candidate.h"
#include "element_state_queue_consume_candidate.h"
#include "element_framework_phase_reset_candidate.h"
#include "element_callback_mask_candidate.h"
#include "element_callback_insert_candidate.h"
#include "callback_inline_detach_candidate.h"
#include "element_callback_storage_candidate.h"
#include "element_callback_list_collect_candidate.h"
#include "test_element_post_update_dispatch_fixture.h"
#include "attribute_callback_prepare_candidate.h"
#include "test_attribute_update_cycle_fixture.h"
#include "attribute_callback_remove_candidate.h"
#include "attribute_callback_register_candidate.h"
#include "test_element_framework_post_queues_fixture.h"
#include "test_element_filtered_callback_register_fixture.h"
#include "test_element_change_propagate_fixture.h"
#include "test_filtered_change_cycle_fixture.h"
#include "test_editor_map_command_fixture.h"
#include "test_editor_map_name_selection_fixture.h"
#include "test_editor_map_fallback_fixture.h"
#include "test_track_clip_at_time_fixture.h"
#include "test_clip_time_frame_bounds_fixture.h"
#include "test_editor_current_time_fixture.h"
#include "test_editor_normal_time_fixture.h"
#include <deque>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <cstdio>
struct Allocator {
    std::int32_t* capacity;
    std::size_t bytes=0;
    unsigned allocs=0,reallocs=0;
    void** allocate(std::size_t n) {
        assert(n==std::size_t(*capacity)*sizeof(void*));bytes=n;++allocs;
        auto p=static_cast<void**>(std::malloc(n));assert(p);return p;
    }
    void** reallocate(void** p,std::size_t n) {
        assert(n==std::size_t(*capacity)*sizeof(void*));bytes=n;++reallocs;
        auto q=static_cast<void**>(std::realloc(p,n));assert(q);return q;
    }
};
struct CallbackAllocator {
    unsigned allocations=0,reallocations=0,frees=0;
    std::size_t lastBytes=0;
    void** allocate(std::size_t n) {lastBytes=n;++allocations;auto p=static_cast<void**>(std::malloc(n));assert(p);return p;}
    void** reallocate(void** p,std::size_t n) {lastBytes=n;++reallocations;auto q=static_cast<void**>(std::realloc(p,n));assert(q);return q;}
    void free(void** p) {++frees;std::free(p);}
};
struct EntryAdapter {
    std::vector<ElementOwnerPoolEntryCandidate> storage;
    std::int32_t used=0,allocated=0;
    ElementOwnerPoolEntryCandidate* cached=nullptr;
    unsigned growths=0;
    std::int32_t count() {return used;}
    std::int32_t capacity() {return allocated;}
    void grow(std::int32_t delta) {assert(used==allocated);allocated+=delta;storage.resize(allocated);++growths;}
    void setCount(std::int32_t value) {used=value;}
    auto* entries() {return storage.data();}
    void setCachedEntries(ElementOwnerPoolEntryCandidate* p) {assert(used>0);cached=p;}
};
struct EntryAllocator {
    using Entry=ElementOwnerPoolEntryCandidate;
    std::size_t bytes=0;
    Entry* allocate(std::size_t n) {bytes=n;auto p=static_cast<Entry*>(std::malloc(n));assert(p);return p;}
    Entry* reallocate(Entry* p,std::size_t n) {bytes=n;auto q=static_cast<Entry*>(std::realloc(p,n));assert(q);return q;}
};
struct QueueAdapter {
    std::vector<std::uint32_t> slots;
    std::int32_t head=-1,tail=-1;
    unsigned wrappedGrowths=0;
    auto capacity() {return static_cast<std::int32_t>(slots.size());}
    auto* data() {return slots.data();}
    void ensureOne() {if(slots.empty())slots.resize(1);}
    void growOne() {if(head!=0)++wrappedGrowths;slots.resize(slots.size()*2);}
};
struct DestroyAdapter {
    struct Owner {bool marked=true;unsigned root=7;} owner;
    bool valid=true,service=true,freed=false;
    std::vector<unsigned> events;
    Owner* resolve(unsigned h) {assert(h==42);events.push_back(1);return valid?&owner:nullptr;}
    bool marked(Owner*) {events.push_back(2);return owner.marked;}
    void cleanupMarked(unsigned h,unsigned mode) {assert(h==42 && mode==1);events.push_back(3);owner.root=9;}
    bool rootServiceAvailable() {events.push_back(4);return service;}
    unsigned root(Owner*) {events.push_back(5);return owner.root;}
    void cleanupRoot(unsigned root,unsigned mode) {assert(root==9 && mode==1);events.push_back(6);}
    void freeOwner(Owner*) {events.push_back(7);freed=true;}
    void releaseHandle(unsigned h) {assert(h==42 && freed);events.push_back(8);}
};
struct ReferenceMarkAdapter {
    struct Owner {} before,after;
    struct Attribute {std::uint16_t flags=0;Owner* owner=nullptr;} attribute;
    unsigned phaseValue=0,queries=0,prepares=0,marks=0;
    auto flags(Attribute* p) {return p->flags;}
    unsigned phase() {++queries;return phaseValue;}
    auto* owner(Attribute* p) {return p->owner;}
    void prepare(Owner* p,Attribute* attr) {assert(p==&before);++prepares;attr->owner=&after;}
    void setFlags(Attribute* p,std::uint16_t f) {p->flags=f;}
    void markOwner(Owner* p) {assert(p==attribute.owner && (attribute.flags&0x4080u)==0x4080u);++marks;}
};
struct StateAdapter {
    struct Attr {std::uint16_t flags;Attr* next;};
    struct Element {std::uint8_t state=0;unsigned handle=42;Attr* first=nullptr;} element;
    std::vector<unsigned> queued;
    auto state(Element* e) {return e->state;}
    auto handle(Element* e) {return e->handle;}
    void enqueue(unsigned h) {assert(!(element.state&1u));queued.push_back(h);}
    void setState(Element* e,std::uint8_t s) {e->state=s;}
    auto* firstAttribute(Element* e) {return e->first;}
    auto flags(Attr* a) {return a->flags;}
    void setFlags(Attr* a,std::uint16_t f) {assert(!(element.state&1u));a->flags=f;}
    auto* nextAttribute(Attr* a) {return a->next;}
};
struct StateQueueAdapter {
    std::vector<std::uint32_t> storage;
    std::int32_t used=0;
    std::uint32_t* cached=nullptr;
    auto count() {return used;}
    auto capacity() {return static_cast<std::int32_t>(storage.size());}
    void grow(std::int32_t delta) {assert(delta==1);storage.resize(storage.empty()?8:storage.size()*2);}
    void setCount(std::int32_t n) {used=n;}
    auto* data() {return storage.data();}
    void setCached(std::uint32_t* p) {assert(used>0);cached=p;}
};
struct ConsumeAdapter:StateQueueAdapter {
    struct Element {unsigned handle;} elements[3]{{1},{2},{3}};
    unsigned phase=0;
    std::vector<unsigned> updates,clears;
    void setPhase(unsigned value) {phase=value;}
    Element* resolve(unsigned h) {assert(phase==5);return h>=1 && h<=3?&elements[h-1]:nullptr;}
    void update(Element* e) {
        updates.push_back(e->handle);
        if(e->handle==1) {
            appendElementStateHandleCandidate(3,*this);
            storage[1]=2; // Consumer must observe current queue contents.
        }
    }
    void clearMarks(Element* e) {assert(updates.back()==e->handle);clears.push_back(e->handle);}
};
struct ResetAdapter {
    unsigned current=0;
    std::vector<unsigned> events;
    auto phase() {return current;}
    void setPhase(unsigned value) {current=value;events.push_back(value);}
    void modelPrepare() {assert(current==1);events.push_back(11);}
    void frameworkPrepare() {assert(current==2);events.push_back(12);}
};
struct CallbackInsertAdapter {
    void* inlineData[2]{};
    std::vector<void*> heap;
    void** cached=nullptr;
    std::int32_t used=0,allocated=2;
    bool borrowed=true;
    std::vector<unsigned> events;
    auto count() {return used;}
    auto capacity() {return allocated;}
    bool external() {return borrowed;}
    int inlineCapacity() {return 2;}
    void detach(int n) {assert(n==2 && borrowed);events.push_back(1);heap.assign(inlineData,inlineData+2);borrowed=false;}
    void grow(int delta) {assert(!borrowed && delta==1);events.push_back(2);allocated*=2;heap.resize(allocated);}
    void setCount(int n) {used=n;}
    void** data() {return borrowed?inlineData:heap.data();}
    void setCached(void** p) {cached=p;}
};
int main() {
    struct RemovalAttribute {unsigned flags=0x6080;};
    struct RemovalAdapter {
        std::vector<PostUpdateDispatchFixture::Node*> freed;
        void freeNode(PostUpdateDispatchFixture::Node* p) {freed.push_back(p);}
        void clearCallbackFlag(RemovalAttribute* p) {p->flags&=~0x2000u;}
    } removal;
    RemovalAttribute removalAttribute;PostUpdateDispatchFixture removalFramework;
    struct RegistrationAdapter {
        PostUpdateDispatchFixture::Node nodes[3]{};
        unsigned allocated=0;
        bool fail=false;
        auto* allocateNode() {if(fail)return static_cast<PostUpdateDispatchFixture::Node*>(nullptr);assert(allocated<3);return &nodes[allocated++];}
        void setCallbackFlag(RemovalAttribute* p) {p->flags|=0x2000u;}
    } registration;
    PostUpdateDispatchFixture::Node* registered=nullptr;
    RemovalAttribute registeredAttribute;registeredAttribute.flags=0x80;
    registerAttributeCallbackCandidate(registered,&removalFramework.paired,static_cast<RemovalAttribute*>(nullptr),registration);
    assert(registration.allocated==0 && registered==nullptr);
    registerAttributeCallbackCandidate(registered,&removalFramework.paired,&registeredAttribute,registration);
    registerAttributeCallbackCandidate(registered,&removalFramework.paired,&registeredAttribute,registration);
    assert(registered==&registration.nodes[1] && registered->next==&registration.nodes[0]);
    assert(registeredAttribute.flags==0x2080 && registered->value==&registeredAttribute);
    registration.fail=true;registeredAttribute.flags=0;
    registerAttributeCallbackCandidate(registered,&removalFramework.paired,&registeredAttribute,registration);
    assert(registered==&registration.nodes[1] && registeredAttribute.flags==0x2000 && registration.allocated==2);
    registration.fail=false;
    registerAttributeCallbackCandidate(registered,static_cast<PostUpdateDispatchFixture::Callback*>(nullptr),&registeredAttribute,registration);
    assert(registered==&registration.nodes[2] && registered->callback==nullptr);
    std::puts("x64 callback registration: null attribute/duplicates/prepend/null allocation flag/null callback passed");
    PostUpdateDispatchFixture::Node removalDirty{&removalFramework.paired,&removalAttribute,nullptr},removalSecond{&removalFramework.paired,&removalAttribute,nullptr},removalFirst{&removalFramework.paired,&removalAttribute,&removalSecond};
    auto* removalRetained=&removalFirst;auto* removalPending=&removalDirty;
    removeAttributeCallbackCandidate(removalRetained,removalPending,&removalFramework.paired,&removalAttribute,removal);
    assert(removalRetained==&removalSecond && removalPending==&removalDirty && removal.freed.size()==1 && removalAttribute.flags==0x6080);
    removeAttributeCallbackCandidate(removalRetained,removalPending,&removalFramework.paired,&removalAttribute,removal);
    assert(removalRetained==nullptr && removal.freed.size()==2 && removalAttribute.flags==0x6080);
    removeAttributeCallbackCandidate(removalRetained,removalPending,&removalFramework.paired,&removalAttribute,removal);
    assert(removalPending==nullptr && removal.freed.size()==3 && removalAttribute.flags==0x4080);
    removalAttribute.flags=0x6080;
    removeAttributeCallbackCandidate(removalRetained,removalPending,&removalFramework.paired,&removalAttribute,removal);
    assert(removalAttribute.flags==0x6080 && removal.freed.size()==3);
    std::puts("x64 callback removal: retained priority/one duplicate/dirty fallback/last flag/missing passed");
    testAttributeUpdateCycleFixture();
    testEditorNormalTimeFixture();
    std::puts("x64 normal time: engine/mode gates/stored sentinel/offset wrap/adjust guards passed");
    testEditorCurrentTimeFixture();
    std::puts("x64 current time: frame/override/clock priority/default rate/fractional and negative frames passed");
    testClipTimeFrameBoundsFixture();
    std::puts("x64 clip frame bounds: separate resolves/missing frame/signed times/wrap/selection passed");
    testTrackClipAtTimeFixture();
    std::puts("x64 track clip selection: half-open signed boundaries/overlap/type/invalid/preparation count passed");
    testEditorMapFallbackFixture();
    std::puts("x64 map fallback protocol: time mapping/clip lookup/type recheck/reverse restore/missing paths passed");
    testEditorMapNameSelectionFixture();
    std::puts("x64 map name selection: supplied/project/secondary/engine/missing primary/order passed");
    testEditorMapCommandFixture();
    std::puts("x64 map command protocol: pending/invalid/fallback/command/helper order passed");
    testFilteredChangeCycleFixture();
    std::puts("x64 filtered change cycle: register/accumulate/queue/dispatch/reset/repeat/cleanup passed");
    testElementChangePropagateFixture();
    std::puts("x64 change propagation: suppression/cyclic incoming refs/exclusions/repeat/stripped mask passed");
    testElementFilteredCallbackRegisterFixture();
    std::puts("x64 filtered registration: 512 mask/state/allocation cases and queue mutation preservation passed");
    testElementFrameworkPostQueuesFixture();
    std::puts("x64 framework post queues: separate snapshots/live contents/reentrant appends/persistent callbacks passed");
    testAttributeSubscriptionLifecycleFixture();
    std::puts("x64 subscription lifecycle: registration/queued removal/dispatch/repeat/final unsubscribe passed");
    std::puts("x64 attribute update cycle: mark/prepare/queue/dispatch/reset/repeat and phase gate passed");
    testElementPostUpdateDispatchFixture();
    std::puts("x64 post-update dispatch: mask snapshots/live mask/paired duplicates/reentrant next pass passed");
    struct CallbackNode {unsigned callback,value;CallbackNode* next;};
    CallbackNode prepareThird{3,10,nullptr},prepareSecond{2,20,&prepareThird},prepareFirst{1,10,&prepareSecond},alreadyDirty{4,30,nullptr};
    CallbackNode* prepareRetained=&prepareFirst;CallbackNode* prepareDirty=&alreadyDirty;
    unsigned enqueues=0;
    auto enqueuePrepared=[&](){++enqueues;};
    prepareAttributeCallbackListCandidate(10u,prepareRetained,prepareDirty,true,enqueuePrepared);
    assert(prepareRetained==&prepareFirst && prepareDirty==&alreadyDirty && enqueues==0);
    prepareAttributeCallbackListCandidate(10u,prepareRetained,prepareDirty,false,enqueuePrepared);
    assert(prepareRetained==&prepareSecond && prepareSecond.next==nullptr && enqueues==1);
    assert(prepareDirty==&prepareThird && prepareThird.next==&prepareFirst && prepareFirst.next==&alreadyDirty);
    prepareAttributeCallbackListCandidate(99u,prepareRetained,prepareDirty,false,enqueuePrepared);
    assert(enqueues==2 && prepareDirty==&prepareThird && prepareRetained==&prepareSecond);
    std::puts("x64 attribute callback prepare: suppression/matching reverse/unmatched order/no-match enqueue passed");
    CallbackNode oldTail{9,90,nullptr},callbackSecond{1,20,nullptr},callbackFirst{1,10,&callbackSecond};
    CallbackNode* dirty=&callbackFirst;CallbackNode* retained=&oldTail;
    std::vector<unsigned> collected,values;
    auto callback=[&](unsigned v){collected.push_back(v);};
    auto value=[&](unsigned v){values.push_back(v);};
    collectRotateElementCallbackListCandidate(dirty,retained,callback,value);
    assert((collected==std::vector<unsigned>{1,1}) && (values==std::vector<unsigned>{10,20}));
    assert(dirty==nullptr && retained==&callbackFirst && callbackSecond.next==&oldTail && oldTail.next==nullptr);
    collected.clear();values.clear();
    collectRotateElementCallbackListCandidate(dirty,retained,callback,value);
    assert(collected.empty() && values.empty() && dirty==nullptr && retained==&callbackFirst);
    CallbackNode newcomer{2,30,nullptr};dirty=&newcomer;
    collectRotateElementCallbackListCandidate(dirty,retained,callback,value);
    assert((collected==std::vector<unsigned>{2}) && (values==std::vector<unsigned>{30}));
    assert(dirty==nullptr && retained==&newcomer && newcomer.next==&callbackFirst);
    std::puts("x64 callback lists: paired values/duplicates/retained concatenation/empty/new pass passed");
    CallbackAllocator combinedAllocator;
    int callbackIdentities[5000]{};
    {
        ElementCallbackStorageCandidate<void*,CallbackAllocator> storage(combinedAllocator);
        for(unsigned i=0;i<5000;++i) {
            storage.append(&callbackIdentities[i]);
            if(i==15)assert(storage.external() && combinedAllocator.allocations==0);
            if(i==16)assert(!storage.external() && storage.capacity()==32 && combinedAllocator.allocations==1);
        }
        assert(storage.capacity()==5008 && combinedAllocator.lastBytes==5008*sizeof(void*));
        storage.insert(1,&callbackIdentities[0]);
        storage.append(nullptr);
        assert(storage.count()==5002 && storage.cachedData()==storage.data());
        assert(storage.data()[0]==&callbackIdentities[0] && storage.data()[1]==&callbackIdentities[0]);
        for(unsigned i=1;i<5000;++i)assert(storage.data()[i+1]==&callbackIdentities[i]);
        assert(storage.data()[5001]==nullptr && combinedAllocator.frees==0);
    }
    assert(combinedAllocator.frees==1);
    {
        ElementCallbackStorageCandidate<void*,CallbackAllocator> small(combinedAllocator);
        small.append(&callbackIdentities[0]);
    }
    assert(combinedAllocator.frees==1);
    std::puts("x64 combined callback storage: 5002 pointers/inline transition/16-slot growth/order/single heap free passed");
    void* inlinePointers[16]{};int callbackIdentity;
    for(auto& p:inlinePointers)p=&callbackIdentity;
    void** detached=inlinePointers;std::int32_t detachCapacity=16,detachGrow=-1;
    Allocator detachAllocator{&detachCapacity};
    detachCallbackInlineStorageCandidate(detached,detachCapacity,detachGrow,16,detachAllocator);
    assert(detached!=inlinePointers && detachGrow==16 && detachAllocator.bytes==128);
    for(unsigned i=0;i<16;++i)assert(detached[i]==inlinePointers[i]);
    growOwnerSymbolPointersCandidate(detached,detachCapacity,detachGrow,1,detachAllocator);
    assert(detachCapacity==32 && detachAllocator.bytes==256);
    auto* owned=detached;
    detachCallbackInlineStorageCandidate(detached,detachCapacity,detachGrow,7,detachAllocator);
    assert(detached==owned && detachGrow==16 && detachAllocator.allocs==1);
    std::free(detached);
    detached=inlinePointers;detachCapacity=0;detachGrow=-1;
    detachCallbackInlineStorageCandidate(detached,detachCapacity,detachGrow,16,detachAllocator);
    assert(detached==nullptr && detachGrow==16 && detachAllocator.allocs==1);
    std::puts("x64 inline detach: full capacity copy/native width/16-slot growth/empty and owned paths passed");
    CallbackInsertAdapter callbacks;int firstCallback,secondCallback;
    void* p1=&firstCallback;void* p2=&secondCallback;
    insertElementCallbackCandidate(0,&p1,callbacks);
    insertElementCallbackCandidate(1,&p2,callbacks);
    insertElementCallbackCandidate(1,&p1,callbacks);
    assert((callbacks.events==std::vector<unsigned>{1,2}));
    assert(callbacks.used==3 && callbacks.cached==callbacks.data());
    assert(callbacks.data()[0]==p1 && callbacks.data()[1]==p1 && callbacks.data()[2]==p2);
    assert(callbacks.inlineData[0]==p1 && callbacks.inlineData[1]==p2);
    std::puts("x64 callback insertion: inline detach-before-growth/order/duplicate/native pointers passed");
    for(unsigned current=0;current<8;++current)for(unsigned required=0;required<8;++required) {
        bool expected=required<4;
        if(required&1u)expected=expected && (current&1u);
        if(required&2u)expected=expected && (current&2u);
        assert(elementCallbackMaskSelectedCandidate(required,current)==expected);
    }
    assert(elementCallbackMaskSelectedCandidate(0,0));
    assert(!elementCallbackMaskSelectedCandidate(0x80000000u,0xffffffffu));
    std::puts("x64 post-update callback mask: subset/zero/unsupported bits passed");
    ResetAdapter reset;resetElementFrameworkPhaseCandidate(reset);
    assert((reset.events==std::vector<unsigned>{1,11,2,12,0}) && reset.current==0);
    reset.events.clear();reset.current=6;resetElementFrameworkPhaseCandidate(reset);
    assert((reset.events==std::vector<unsigned>{0}) && reset.current==0);
    std::puts("x64 framework phase reset: phase0 preparation and phase6 reset passed");
    ConsumeAdapter consume;
    appendElementStateHandleCandidate(1,consume);
    appendElementStateHandleCandidate(0xffffffffu,consume);
    appendElementStateHandleCandidate(0xffffffffu,consume);
    consumeElementStateQueueCandidate(consume);
    assert((consume.updates==std::vector<unsigned>{1,2}) && consume.clears==consume.updates);
    assert(consume.used==0 && consume.phase==5 && consume.storage[3]==3);
    ConsumeAdapter empty;consumeElementStateQueueCandidate(empty);
    assert(empty.phase==5 && empty.used==0 && empty.updates.empty());
    std::puts("x64 state queue consume: snapshot/live reads/invalid skip/update-reset order passed");
    StateQueueAdapter stateQueue;
    for(unsigned i=0;i<5000;++i)appendElementStateHandleCandidate(i,stateQueue);
    appendElementStateHandleCandidate(42,stateQueue);
    appendElementStateHandleCandidate(42,stateQueue);
    appendElementStateHandleCandidate(0xffffffffu,stateQueue);
    assert(stateQueue.used==5003 && stateQueue.cached==stateQueue.storage.data());
    for(unsigned i=0;i<5000;++i)assert(stateQueue.storage[i]==i);
    assert(stateQueue.storage[5000]==42 && stateQueue.storage[5001]==42 && stateQueue.storage[5002]==0xffffffffu);
    std::puts("x64 element state queue: 5003 handles/order/duplicates/sentinel/cache passed");
    StateAdapter state;
    StateAdapter::Attr second{0xffffu,nullptr},first{0x4080u,&second};
    state.element.first=&first;state.element.state=0x10;
    markElementStateCandidate(&state.element,state);
    markElementStateCandidate(&state.element,state);
    assert(state.element.state==0x13 && state.queued.size()==1 && state.queued[0]==42);
    clearElementAttributeMarkCandidate(&state.element,state);
    assert(state.element.state==0x12 && first.flags==0x4000 && second.flags==0xff7f);
    markElementStateCandidate(&state.element,state);
    assert(state.queued.size()==2 && state.element.state==0x13);
    std::puts("x64 element state: single enqueue/reset/requeue and attribute flag preservation passed");
    for(unsigned phase=0;phase<7;++phase)for(auto flags:{0u,0x20u,0x400u,0x2000u,0x2080u}) {
        ReferenceMarkAdapter a;a.phaseValue=phase;
        a.attribute={static_cast<std::uint16_t>(flags),&a.before};
        const bool allowed=!(flags&0x20u) && (phase<=2 || (phase==4 && !(flags&0x400u)));
        assert(markAttributeReferenceCandidate(&a.attribute,a)==allowed);
        assert(a.queries==((flags&0x20u)?0u:1u));
        assert(a.marks==(allowed?1u:0u));
        assert(a.prepares==((allowed && (flags&0x2000u) && !(flags&0x80u))?1u:0u));
        assert(a.attribute.flags==(allowed?(flags|0x4080u):flags));
        if(a.prepares)assert(a.attribute.owner==&a.after);
    }
    std::puts("x64 attribute reference marking: phase/flag gates/changed owner ordering passed");
    assert(elementCleanupImmediateReleaseCandidate(0,0,0xffffffffu));
    assert(elementCleanupImmediateReleaseCandidate(2,0xffffffffu,0xc0000000u));
    assert(!elementCleanupImmediateReleaseCandidate(2,0,0));
    for(unsigned bit=0;bit<30;++bit)
        assert(!elementCleanupImmediateReleaseCandidate(2,0xffffffffu,1u<<bit));
    assert(!elementCleanupImmediateReleaseCandidate(1,0xffffffffu,0));
    assert(!elementCleanupImmediateReleaseCandidate(0xffffffffu,0xffffffffu,0));
    std::puts("x64 element cleanup release decision: mode and all30 relevant bits passed");
    testElementOwnerLifecycleFixture();
    DestroyAdapter destroy;destroyElementOwnerProtocolCandidate(42,destroy);
    assert((destroy.events==std::vector<unsigned>{1,2,3,4,5,6,7,8}));
    destroy=DestroyAdapter{};destroy.valid=false;destroyElementOwnerProtocolCandidate(42,destroy);
    assert((destroy.events==std::vector<unsigned>{1}));
    destroy=DestroyAdapter{};destroy.owner.marked=false;destroy.service=false;
    destroyElementOwnerProtocolCandidate(42,destroy);
    assert((destroy.events==std::vector<unsigned>{1,2,4,7,8}));
    std::puts("x64 owner teardown: guarded lazy paths/updated root/free-before-release passed");
    std::puts("x64 owner lifecycle: 1000 owners/FIFO reuse/stale handles/2047-generation retirement passed");
    static_assert(sizeof(void*)==8,"x64 required");
    std::int32_t cap=0;void** p=nullptr;Allocator a{&cap};
    int identities[5000]{};
    for(std::int32_t i=0;i<5000;++i) {
        if(i+1>cap)growOwnerSymbolPointersCandidate(p,cap,0,i+1-cap,a);
        p[i]=&identities[i];
        for(std::int32_t j=0;j<=i;++j)assert(p[j]==&identities[j]);
    }
    assert(cap==8192 && a.bytes==65536 && a.allocs==1 && a.reallocs==10);
    const auto saved=p;
    growOwnerSymbolPointersCandidate(p,cap,-1,1,a);
    assert(p==saved && cap==8192 && a.reallocs==10);
    std::free(p);p=nullptr;cap=0;
    growOwnerSymbolPointersCandidate(p,cap,7,8,a);
    assert(cap==14 && a.bytes==112);
    growOwnerSymbolPointersCandidate(p,cap,7,1,a);
    assert(cap==21 && a.bytes==168);
    std::free(p);
    cap=0x40000000;p=nullptr;
    try {growOwnerSymbolPointersCandidate(p,cap,0,1,a);assert(false);}
    catch(const std::overflow_error&) {assert(cap==0x40000000 && p==nullptr);}
    EntryAdapter entries;ElementOwnerCounterCandidate owners[2];
    assert(insertElementOwnerEntryCandidate(0,entries)==0);
    entries.storage[0]={7,&owners[0]};
    insertElementOwnerEntryCandidate(1,entries);
    entries.storage[1]={9,&owners[1]};
    insertElementOwnerEntryCandidate(1,entries);
    assert(entries.storage[0].generationAndFlags==7 && entries.storage[0].object==&owners[0]);
    assert(entries.storage[1].generationAndFlags==0 && entries.storage[1].object==nullptr);
    assert(entries.storage[2].generationAndFlags==9 && entries.storage[2].object==&owners[1]);
    assert(entries.cached==entries.storage.data() && entries.used==3 && entries.growths==3);
    entries.allocated=4;entries.storage.resize(4);
    insertElementOwnerEntryCandidate(0,entries);
    assert(entries.storage[0].object==nullptr && entries.storage[1].object==&owners[0]);
    assert(entries.storage[3].object==&owners[1] && entries.growths==3);
    ElementOwnerPoolEntryCandidate* nativeEntries=nullptr;
    std::int32_t nativeCapacity=0;EntryAllocator entryAllocator;
    for(std::int32_t i=0;i<5000;++i) {
        if(i+1>nativeCapacity)growRecoveredStorageCandidate(nativeEntries,nativeCapacity,0,
            i+1-nativeCapacity,4,entryAllocator);
        if(i==0)assert(nativeCapacity==4);
        nativeEntries[i]={static_cast<std::uint32_t>(i),&owners[i%2]};
        for(std::int32_t j=0;j<=i;++j)
            assert(nativeEntries[j].generationAndFlags==static_cast<std::uint32_t>(j) && nativeEntries[j].object==&owners[j%2]);
    }
    assert(nativeCapacity==8192 && entryAllocator.bytes==8192*sizeof(*nativeEntries));
    std::free(nativeEntries);
    ElementOwnerPoolEntryCandidate binding[2]={{3,&owners[0]},{0x80000004u,nullptr}};
    std::uint32_t live=1;
    bindElementOwnerCandidate(0xffffffffu,nullptr,0,live,&owners[1]);
    bindElementOwnerCandidate((3u<<20)|2u,binding,2,live,&owners[1]);
    bindElementOwnerCandidate(2u<<20,binding,2,live,&owners[1]);
    assert(binding[0].object==&owners[0] && live==1);
    assert(resolveElementOwnerCandidate((4u<<20)|1u,binding,2)==nullptr);
    bindElementOwnerCandidate(0x80000000u|(4u<<20)|1u,binding,2,live,&owners[1]);
    assert(live==2 && binding[1].generationAndFlags==4 && binding[1].object==&owners[1]);
    assert(resolveElementOwnerCandidate((4u<<20)|1u,binding,2)==&owners[1]);
    bindElementOwnerCandidate((4u<<20)|1u,binding,2,live,nullptr);
    assert(live==2 && binding[1].object==nullptr && binding[1].generationAndFlags==4);
    binding[0].generationAndFlags=0x80000003u;live=0xffffffffu;
    bindElementOwnerCandidate(3u<<20,binding,2,live,&owners[0]);
    assert(live==0 && binding[0].generationAndFlags==3);
    std::puts("x64 owner binding: stale rejection/reactivation/null assignment/count wrap passed");
    std::vector<std::uint32_t> recycled;
    auto enqueue=[&](std::uint32_t index){assert(binding[index].object==nullptr);recycled.push_back(index);};
    live=1;binding[0]={3,&owners[0]};
    releaseElementOwnerCandidate(99u<<20,binding,2,live,enqueue);
    assert(live==0 && binding[0].generationAndFlags==0x80000004u && recycled.size()==1);
    assert(resolveElementOwnerCandidate(3u<<20,binding,2)==nullptr);
    releaseElementOwnerCandidate(3u<<20,binding,2,live,enqueue);
    assert(live==0 && binding[0].generationAndFlags==0x80000005u && recycled.size()==2);
    binding[1]={2046,&owners[1]};live=1;
    releaseElementOwnerCandidate(1,binding,2,live,enqueue);
    assert(live==0 && binding[1].generationAndFlags==0x800007ffu && recycled.size()==2);
    releaseElementOwnerCandidate(0xffffffffu,binding,2,live,enqueue);
    assert(recycled.size()==2);
    binding[0]={0x7fffffffu,&owners[0]};live=0;
    releaseElementOwnerCandidate(0,binding,2,live,enqueue);
    assert(live==0xffffffffu && binding[0].generationAndFlags==0x80000000u && recycled.size()==3);
    std::puts("x64 owner release: generation/stale-input/repeat/retirement/wrap passed");
    QueueAdapter queue;std::deque<std::uint32_t> expected;
    auto push=[&](std::uint32_t value) {enqueueElementOwnerSlotCandidate(value,queue);expected.push_back(value);};
    auto pop=[&]() {assert(dequeueElementOwnerSlotCandidate(queue)==expected.front());expected.pop_front();};
    for(unsigned i=0;i<4;++i)push(i);
    pop();pop();push(4);push(5);push(6);
    assert(queue.wrappedGrowths==1 && queue.capacity()==8);
    while(!expected.empty())pop();
    assert(queue.head==-1 && queue.tail==-1);
    std::uint32_t random=17;
    for(unsigned i=0;i<20000;++i) {
        random=random*1664525u+1013904223u;
        if(expected.empty() || random%5<3)push(i);else pop();
    }
    while(!expected.empty())pop();
    assert(queue.head==-1 && queue.tail==-1);
    std::puts("x64 recycled-slot ring: wrapped growth and 20000 FIFO operations passed");
    std::puts("x64 owner entry growth: 5000 metadata/pointer pairs retained with native stride");
    std::puts("x64 owner entry insertion: append/interior/front metadata and widened pointer preservation passed");
    std::puts("x64 pointer growth: 5000 retained pointers, width/order/boundaries passed; original parity unverified");
}
