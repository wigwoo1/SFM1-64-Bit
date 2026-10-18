#include "element_owner_registration_candidate.h"
#include "element_owner_graph_walk_candidate.h"
#include "element_owner_creation_candidate.h"
#include "element_owner_lookup_candidate.h"
#include "owner_symbol_lookup_protocol_candidate.h"
#include "owner_symbol_chain_candidate.h"
#include "owner_symbol_publication_protocol_candidate.h"
#include "owner_symbol_node_link_candidate.h"
#include "owner_symbol_primary_publish_candidate.h"
#include "test_owner_symbol_composition_fixture.h"
#include "owner_symbol_arena_fit_candidate.h"
#include "owner_symbol_arena_record_candidate.h"
#include "owner_symbol_hash_candidate.h"
#include "element_owner_path_candidate.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <set>
#include <cstring>
#include <stdexcept>
struct PrimaryPublishAdapter {
    struct Bucket {unsigned primary=0,secondary=42;};
    std::array<Bucket,2048> buckets;
    bool changed=true,locked=false;
    unsigned reads=0,copies=0;
    bool dirty() {return changed;}
    Bucket* bucket(unsigned index) {assert(index==reads);return &buckets[index];}
    void enterRead(Bucket*) {assert(!locked);locked=true;++reads;}
    unsigned secondary(Bucket* b) {assert(locked);return b->secondary;}
    void setPrimary(Bucket* b,unsigned head) {assert(locked);b->primary=head;++copies;}
    void leaveRead(Bucket*) {assert(locked);locked=false;}
    void clearDirty() {assert(reads==2048 && copies==2048 && !locked);changed=false;}
};
struct SymbolProtocolAdapter {
    struct Node {} node;
    struct Bucket {} value;
    unsigned searches=0;
    bool firstHit=false,secondHit=true;
    std::vector<int> events;
    int makeKey(const char*) {events.push_back(1);return 0x1234;}
    std::uint32_t hash(int) {return 0x1234;}
    Bucket* bucket(std::uint32_t index) {assert(index==0x234);return &value;}
    int root(Bucket*) {return static_cast<int>(searches);}
    unsigned context(Bucket*) {return 17;}
    Node* search(int,int rootValue,unsigned extra) {
        assert(rootValue==(searches?17:0));
        assert(extra==(searches?1u:0u));
        events.push_back(2);return (searches++?secondHit:firstHit)?&node:nullptr;
    }
    void enter(Bucket*) {events.push_back(3);}
    void leave(Bucket*) {events.push_back(4);}
    std::uint32_t symbol(Node*) {events.push_back(5);return 42;}
};
struct PublicationAdapter {
    struct Node {} node;struct Bucket {} value;
    unsigned searches=0,hitAt=3;
    std::vector<int> events;
    Bucket* bucket(unsigned) {return &value;}
    unsigned hash(int) {return 42;}
    int root(Bucket*) {return 5;}
    int secondary(Bucket*) {return 9;}
    Node* search(int,int head,unsigned stop) {
        assert(head==(searches?9:5) && stop==(searches?5u:0u));
        events.push_back(1);return searches++==hitAt?&node:nullptr;
    }
    void enterRead(Bucket*) {events.push_back(2);}
    void leaveRead(Bucket*) {events.push_back(3);}
    void enterWrite(Bucket*) {events.push_back(4);}
    void leaveWrite(Bucket*) {events.push_back(5);}
    Node* allocateLink(int,Bucket*) {events.push_back(6);return &node;}
    void storeKey(Node*,int) {events.push_back(7);}
};
struct NodeLinkAdapter {
    struct Node {Node* next=nullptr;int key=0;} node,old;
    struct Bucket {Node* head;} bucket{&old};
    std::vector<int> events;
    void markDirty() {events.push_back(1);}
    void enterAllocator() {events.push_back(2);}
    Node* allocateNode() {events.push_back(3);return &node;}
    void leaveAllocator() {events.push_back(4);}
    Node* secondary(Bucket* b) {return b->head;}
    void setNext(Node* n,Node* next) {events.push_back(5);n->next=next;}
    void setSecondary(Bucket* b,Node* n) {events.push_back(6);b->head=n;}
    void setSearchKey(Node* n,int key) {events.push_back(7);n->key=key;}
};
void testSymbolPublication() {
    PublicationAdapter a;
    assert(publishOwnerSymbolProtocolCandidate(42,a)==&a.node);
    assert((a.events==std::vector<int>{1,2,1,3,4,1,6,7,5}));
    PublicationAdapter b;b.hitAt=2;
    assert(publishOwnerSymbolProtocolCandidate(42,b)==&b.node);
    assert((b.events==std::vector<int>{1,2,1,3,4,1,5}));
    PublicationAdapter c;c.hitAt=0;
    assert(publishOwnerSymbolProtocolCandidate(42,c)==&c.node);
    assert((c.events==std::vector<int>{1}));
}
void testSymbolProtocol() {
    SymbolProtocolAdapter a;
    assert(lookupOwnerSymbolProtocolCandidate(nullptr,a)==std::numeric_limits<std::uintptr_t>::max() && a.events.empty());
    assert(lookupOwnerSymbolProtocolCandidate("fixture",a)==42);
    assert((a.events==std::vector<int>{1,2,3,2,4,5}));
    SymbolProtocolAdapter b;b.firstHit=true;
    assert(lookupOwnerSymbolProtocolCandidate("fixture",b)==42);
    assert((b.events==std::vector<int>{1,2,5}));
    SymbolProtocolAdapter c;c.secondHit=false;
    assert(lookupOwnerSymbolProtocolCandidate("fixture",c)==std::numeric_limits<std::uintptr_t>::max());
    assert((c.events==std::vector<int>{1,2,3,2,4}));
}
struct NamedOwnerEntry {
    std::uint32_t generationAndFlags;
    ElementOwnerMetadataCandidate* object;
};
struct LookupAdapter {
    NamedOwnerEntry* entries;
    std::int32_t count;
    std::uint32_t symbol=42;
    unsigned poolReads=0;
    void normalize(char* out,std::size_t size,const char* source) {
        copyElementOwnerPathCandidate(out,static_cast<int>(size),source);
    } // Fixture normalization only.
    std::uint32_t findSymbol(const char*) {return symbol;}
    std::int32_t poolCount() {++poolReads;return count;}
    NamedOwnerEntry* poolTable() {return entries;}
    ElementOwnerMetadataCandidate* resolveOwner(std::uint32_t handle) {
        const auto index=handle&0xfffffu;
        if (index>=static_cast<std::uint32_t>(count))return nullptr;
        const auto& entry=entries[index];
        if (entry.generationAndFlags!=((handle>>20)&0x7ffu))return nullptr;
        return entry.object;
    }
    std::uint32_t nameSymbol(ElementOwnerMetadataCandidate* owner) {return owner->nameSymbol;}
};
void testOwnerLookup() {
    ElementOwnerMetadataCandidate a,b;a.nameSymbol=b.nameSymbol=42;
    NamedOwnerEntry entries[]={{1,nullptr},{2,&a},{3,&b}};
    LookupAdapter adapter{entries,3};
    assert(lookupElementOwnerCandidate("fixture",adapter)==0x200001u);
    entries[1].generationAndFlags|=0x80000000u;
    assert(lookupElementOwnerCandidate("fixture",adapter)==0x300002u);
    b.nameSymbol=43;
    assert(lookupElementOwnerCandidate("fixture",adapter)==0xffffffffu);
    adapter.symbol=0xffffffffu;const auto previous=adapter.poolReads;
    assert(lookupElementOwnerCandidate("missing",adapter)==0xffffffffu);
    assert(adapter.poolReads==previous);
    adapter.symbol=42;adapter.count=0;
    assert(lookupElementOwnerCandidate("fixture",adapter)==0xffffffffu);
}
struct AbsolutePathAdapter {
    unsigned cwdCalls=0;
    bool currentDirectory(char* output,int capacity) {
        ++cwdCalls;copyElementOwnerPathCandidate(output,capacity,"Q:/Fixture");return true;
    }
    void diagnostic(const char* reason) {throw std::runtime_error(reason);}
};
void testAbsoluteOwnerPaths() {
    AbsolutePathAdapter adapter;char out[260];
    makeElementOwnerAbsolutePathCandidate(out,260,"C:/A/../B",nullptr,adapter);
    assert(std::strcmp(out,"C:\\B")==0 && adapter.cwdCalls==0);
    makeElementOwnerAbsolutePathCandidate(out,260,"../B","D:/A",adapter);
    assert(std::strcmp(out,"D:\\B")==0 && adapter.cwdCalls==0);
    makeElementOwnerAbsolutePathCandidate(out,260,"B","A",adapter);
    assert(std::strcmp(out,"Q:\\Fixture\\A\\B")==0 && adapter.cwdCalls==1);
    makeElementOwnerAbsolutePathCandidate(out,260,"B",nullptr,adapter);
    assert(std::strcmp(out,"Q:\\Fixture\\B")==0 && adapter.cwdCalls==2);
    makeElementOwnerAbsolutePathCandidate(out,260,"C:/ABCDE",nullptr,adapter);
    makeElementOwnerAbsolutePathCandidate(out,5,"C:/ABCDE",nullptr,adapter);
    assert(std::strcmp(out,"C:\\A")==0);
    bool capacityFailed=false;
    try {makeElementOwnerAbsolutePathCandidate(out,3,"B","C:/A",adapter);}
    catch(const std::runtime_error& e) {capacityFailed=std::strcmp(e.what(),"capacity")==0;}
    assert(capacityFailed);
}
void testPrimaryOwnerPathCleanup() {
    struct Case {const char* input;const char* output;bool result;};
    const Case cases[]={
        {"","",true},{"./A//B","A\\B",true},
        {"C:/A/../B","C:\\B",true},{"/A/../B","\\B",true},
        {"A/../B","A/../B",false},{"../A","../A",false},
        {"A./B","AB",true},{"A/.","A",true},{"/.","\\.",true},
        {"//server///share","\\server\\share",true},
        {"./A//../B","A/../B",false},
        {"C:/A/B/../../D","C:\\D",true},
        {"C:/A/..","C:",true},{"a..b/c","a..b\\c",true}
    };
    for (const auto& c:cases) {
        char buffer[260];std::memcpy(buffer,c.input,std::strlen(c.input)+1);
        assert(cleanupElementOwnerPathCandidate(buffer,'\\')==c.result);
        assert(std::strcmp(buffer,c.output)==0);
    }
}
struct CreationAdapter {
    ElementOwnerMetadataCandidate record;
    std::vector<int> events;
    std::uint32_t existing=0xffffffffu;
    bool failAllocation=false;
    ElementOwnerMetadataCandidate* bound=nullptr;
    void normalize(char* out,std::size_t size,const char* name) {
        events.push_back(1);const auto length=std::strlen(name);
        assert(length<size);std::memcpy(out,name,length+1);
    } // Fixture only; original normalization pending.
    std::uint32_t lookup(const char*) {events.push_back(2);return existing;}
    void mark(std::uint32_t h) {events.push_back(3);assert(h==existing);}
    std::uint32_t reserve() {events.push_back(4);return 0x400002u;}
    ElementOwnerMetadataCandidate* allocate() {
        events.push_back(5);return failAllocation?nullptr:&record;
    }
    std::uint32_t intern(const char*) {events.push_back(6);return 42;}
    void bind(std::uint32_t h,ElementOwnerMetadataCandidate* p) {
        events.push_back(7);assert(h==0x400002u);bound=p;
    }
};
struct PathAdapter {
    std::vector<int> events;
    void normalizeLocal(char* out,int,const char* source) {
        events.push_back(1);std::memcpy(out,source,std::strlen(source)+1);
    }
    void copy(char* out,const char* source,int) {
        events.push_back(2);std::memcpy(out,source,std::strlen(source)+1);
    }
    void servicePath(char* out,int) {events.push_back(3);out[0]='X';out[1]=0;}
    void relative(char* out,int,const char* source,const char* service) {
        events.push_back(4);assert(service[0]=='X');
        std::memcpy(out,source,std::strlen(source)+1);
    }
}; // Dispatch fixture only: does not implement unresolved bundled helpers.
void testOwnerCreation() {
    CreationAdapter a;
    assert(createElementOwnerCandidate(nullptr,a)==0xffffffffu);
    assert(createElementOwnerCandidate("",a)==0xffffffffu && a.events.empty());
    assert(createElementOwnerCandidate("fixture.dmx",a)==0x400002u);
    assert((a.events==std::vector<int>{1,2,4,5,6,7}));
    assert(a.bound==&a.record && a.record.nameSymbol==42);
    assert(a.record.rootHandle==0xffffffffu && a.record.secondarySymbol==0xffffffffu);
    assert(a.record.marked==1 && a.record.elementCount==0 && a.record.field14==0);
    a.events.clear();a.existing=17;
    assert(createElementOwnerCandidate("fixture.dmx",a)==17);
    assert((a.events==std::vector<int>{1,2,3}));
    a.events.clear();a.existing=0xffffffffu;a.failAllocation=true;
    assert(createElementOwnerCandidate("fixture.dmx",a)==0x400002u && a.bound==nullptr);
    assert((a.events==std::vector<int>{1,2,4,5,7}));
}
struct WalkAttribute {
    std::uint32_t flags=1;
    std::vector<std::uint32_t> references;
    WalkAttribute* next=nullptr;
};
struct WalkElement {
    std::uint32_t handle,owner;
    WalkAttribute* first=nullptr;
    bool baseElement=true;
};
struct WalkAdapter {
    std::vector<WalkElement*> elements;
    std::set<std::uint32_t> seen;
    std::vector<std::uint32_t> order;
    auto owner(WalkElement* e) {return e->owner;}
    auto handle(WalkElement* e) {return e->handle;}
    bool visited(std::uint32_t h) {return seen.count(h)!=0;}
    void markVisited(std::uint32_t h) {seen.insert(h);}
    void transition(WalkElement* e,std::uint32_t owner) {
        e->owner=owner;order.push_back(e->handle);
    }
    auto firstAttribute(WalkElement* e) {return e->first;}
    auto nextAttribute(WalkAttribute* a) {return a->next;}
    auto flags(WalkAttribute* a) {return a->flags;}
    auto scalarReference(WalkAttribute* a) {return a->references.at(0);}
    auto arrayCount(WalkAttribute* a) {return static_cast<std::int32_t>(a->references.size());}
    auto arrayReference(WalkAttribute* a,std::int32_t i) {return a->references.at(i);}
    WalkElement* resolve(std::uint32_t h) {
        for (auto* e:elements) if(e->handle==h)return e;
        return nullptr;
    }
    bool isElement(WalkElement* e) {return e->baseElement;}
};
void testOwnerWalk() {
    WalkAttribute a{0x1001,{2},nullptr}, b{15,{1,3,99,4},nullptr};
    WalkElement root{1,10,&a,true}, child{2,10,&b,true};
    WalkElement foreign{3,11,nullptr,true},wrongType{4,10,nullptr,false};
    WalkAdapter adapter{{&root,&child,&foreign,&wrongType},{},{}};
    transitionElementOwnerGraphCandidate(&root,20,0,10,1,adapter);
    assert((adapter.order==std::vector<std::uint32_t>{1,2}));
    assert(root.owner==20 && child.owner==20 && foreign.owner==11);
    root.owner=child.owner=10;adapter.seen.clear();adapter.order.clear();
    transitionElementOwnerGraphCandidate(&root,20,1,10,1,adapter);
    assert((adapter.order==std::vector<std::uint32_t>{1}));
    root.owner=child.owner=10;adapter.seen.clear();adapter.order.clear();
    a.flags=0x801;b.flags=0x80f;
    transitionElementOwnerGraphCandidate(&root,20,2,10,0,adapter);
    assert((adapter.order==std::vector<std::uint32_t>{1,2,3}));
    root.owner=10;adapter.seen.clear();adapter.order.clear();
    transitionElementOwnerGraphCandidate(&root,20,3,10,0,adapter);
    assert((adapter.order==std::vector<std::uint32_t>{1}));
    adapter.seen.clear();adapter.order.clear();
    transitionElementOwnerGraphCandidate(&root,30,0,10,1,adapter);
    assert(adapter.seen.empty() && adapter.order.empty()); // Gate before visit.
    assert(!ownerTraversalAttributeCandidate(0x1801,2));
    assert(!ownerTraversalAttributeCandidate(1,2));
    assert(!ownerTraversalAttributeCandidate(0x801,0xffffffffu));
}
int main() {
    static_assert(sizeof(void*) == 8);
    testOwnerWalk();
    testOwnerCreation();
    testOwnerLookup();
    testSymbolProtocol();
    testSymbolPublication();
    testOwnerSymbolCompositionFixture();
    OwnerSymbolArenaBlockCandidate block0{16,9},block1{16,8},block2{16,0};
    const OwnerSymbolArenaBlockCandidate* blocks[]={&block0,&block1,&block2};
    assert(findOwnerSymbolArenaFitCandidate(blocks,3,8)==1);
    assert(findOwnerSymbolArenaFitCandidate(blocks,3,17)==-1);
    assert(findOwnerSymbolArenaFitCandidate(blocks,3,-1)==0);
    assert(findOwnerSymbolArenaFitCandidate(nullptr,0,8)==-1);
    assert(findOwnerSymbolArenaFitCandidate(nullptr,-1,8)==-1);
    block0={0,1};
    assert(findOwnerSymbolArenaFitCandidate(blocks,3,0)==1);
    assert(ownerSymbolRecordBytesCandidate(0)==8);
    assert(ownerSymbolRecordBytesCandidate(3)==8);
    assert(ownerSymbolRecordBytesCandidate(4)==12);
    assert(ownerSymbolNewBlockBytesCandidate(8)==2048);
    assert(ownerSymbolNewBlockBytesCandidate(2040)==2052);
    char arena[64];std::memset(arena,0x55,sizeof(arena));std::uint32_t used=0;
    assert(writeOwnerSymbolArenaRecordCandidate(arena,used,8,42,"abc",4)==arena);
    assert(used==8 && std::strcmp(arena+4,"abc")==0);
    std::uint32_t storedHash=0;std::memcpy(&storedHash,arena,4);assert(storedHash==42);
    assert(writeOwnerSymbolArenaRecordCandidate(arena,used,12,43,"abcd",5)==arena+8);
    assert(used==20 && std::strcmp(arena+12,"abcd")==0);
    assert(arena[17]==0x55 && arena[20]==0x55); // Padding/tail not rewritten.
    PrimaryPublishAdapter primary;
    primary.buckets[17].secondary=0;
    publishOwnerSymbolPrimaryCandidate(primary);
    assert(!primary.changed && primary.reads==2048 && primary.buckets[17].primary==0);
    assert(primary.buckets[2047].primary==42);
    publishOwnerSymbolPrimaryCandidate(primary);
    assert(primary.reads==2048); // Clean table skips locks/copy.
    NodeLinkAdapter nodeLink;
    assert(linkOwnerSymbolNodeCandidate(42,&nodeLink.bucket,nodeLink)==&nodeLink.node);
    assert(nodeLink.bucket.head==&nodeLink.node && nodeLink.node.next==&nodeLink.old);
    assert(nodeLink.node.key==42 && (nodeLink.events==std::vector<int>{1,2,3,4,5,6,7}));
    std::array<std::uint32_t,256> identityTable{};
    for (unsigned i=0;i<256;++i) identityTable[i]=i;
    assert(hashOwnerSymbolCandidate("",identityTable)==0);
    assert(hashOwnerSymbolCandidate("a",identityTable)==0x6100u);
    assert(hashOwnerSymbolCandidate("ab",identityTable)==0x6103u);
    assert(hashOwnerSymbolCandidate("abc",identityTable)==0x6003u);
    assert(hashOwnerSymbolCandidate("\x80\xff",identityTable)==0x807fu);
    OwnerSymbolKeyCandidate wanted{42,"name"},differentHash{43,"name"},caseName{42,"Name"};
    OwnerSymbolChainNodeCandidate last{&wanted,nullptr},middle{&caseName,&last},first{&differentHash,&middle};
    assert(findOwnerSymbolChainCandidate(wanted,&first,nullptr)==&last);
    assert(findOwnerSymbolChainCandidate(wanted,&first,&last)==nullptr);
    assert(findOwnerSymbolChainCandidate(wanted,&last,&last)==nullptr);
    assert(compareOwnerSymbolBytesCandidate("\x80","\x7f")==1);
    assert(compareOwnerSymbolBytesCandidate("a","ab")==-1);
    testPrimaryOwnerPathCleanup();
    testAbsoluteOwnerPaths();
    char appendPath[8]="A";unsigned capacityErrors=0;
    appendElementOwnerSlashCandidate(appendPath,3,[&](const char*) {++capacityErrors;});
    assert(std::strcmp(appendPath,"A\\")==0 && capacityErrors==0);
    appendElementOwnerSlashCandidate(appendPath,1,[&](const char*) {++capacityErrors;});
    assert(capacityErrors==0); // Already terminated with separator; no size check.
    appendPath[1]=0;
    appendElementOwnerSlashCandidate(appendPath,2,[&](const char* p) {
        ++capacityErrors;assert(std::strcmp(p,"A")==0);
    }); // Physically8-byte buffer: safely observe original return-after-error path.
    assert(capacityErrors==1 && std::strcmp(appendPath,"A\\")==0);
    appendPath[0]=0;
    appendElementOwnerSlashCandidate(appendPath,0,[&](const char*) {++capacityErrors;});
    assert(capacityErrors==1 && appendPath[0]==0);
    char failedRoot[]="./A//../B";unsigned diagnostics=0;
    finishElementOwnerAbsolutePathCandidate(failedRoot,[&] {++diagnostics;});
    assert(diagnostics==1 && std::strcmp(failedRoot,"A\\..\\B")==0);
    char goodRoot[]="C:/A/../B";
    finishElementOwnerAbsolutePathCandidate(goodRoot,[&] {++diagnostics;});
    assert(diagnostics==1 && std::strcmp(goodRoot,"C:\\B")==0);
    assert(!elementOwnerPathAbsoluteCandidate(""));
    assert(!elementOwnerPathAbsoluteCandidate("fixture.dmx"));
    assert(elementOwnerPathAbsoluteCandidate("C:relative.dmx"));
    assert(elementOwnerPathAbsoluteCandidate("1:relative.dmx"));
    assert(elementOwnerPathAbsoluteCandidate("/fixture.dmx"));
    assert(elementOwnerPathAbsoluteCandidate("\\fixture.dmx"));
    assert(!elementOwnerPathAbsoluteCandidate(":fixture.dmx"));
    char casePath[]="AZaz/\\09\x80";
    assert(lowercaseElementOwnerPathAsciiCandidate(casePath)==casePath);
    assert(std::strcmp(casePath,"azaz/\\09\x80")==0);
    char duplicatePath[]="//A///B";
    cleanupElementOwnerDuplicateSeparatorsCandidate(duplicatePath);
    assert(std::strcmp(duplicatePath,"//A//B")==0);
    char emptyPath[]="";
    cleanupElementOwnerDuplicateSeparatorsCandidate(emptyPath);
    assert(emptyPath[0]==0);
    PathAdapter pathAdapter;char pathOutput[260];
    normalizeElementOwnerPathCandidate(pathOutput,260,"C:fixture",pathAdapter);
    assert((pathAdapter.events==std::vector<int>{1,2}));
    pathAdapter.events.clear();
    normalizeElementOwnerPathCandidate(pathOutput,260,"fixture",pathAdapter);
    assert((pathAdapter.events==std::vector<int>{1,3,4}));
    ElementOwnerCounterCandidate object{0xffffffffu};
    ElementOwnerPoolEntryCandidate entries[2]{{0,nullptr},{0x7ff,&object}};
    const std::uint32_t handle = 0x7ff00001u;
    assert(elementOwnerCountCandidate(handle,entries,2)==0xffffffffu);
    assert(elementOwnerCountCandidate(0xffffffffu,nullptr,0)==0);
    assert(elementOwnerCountCandidate(0x7fe00001u,entries,2)==0);
    registerElementOwnerCandidate(17,handle,entries,2);
    assert(object.count == 0); // Counter wrapping.
    registerElementOwnerCandidate(99,handle | 0x80000000u,entries,2);
    assert(object.count == 1); // Handle high bit ignored; first arg unused.
    registerElementOwnerCandidate(17,0xffffffffu,nullptr,0);
    registerElementOwnerCandidate(17,0x7ff00002u,entries,2);
    registerElementOwnerCandidate(17,0x7fe00001u,entries,2);
    assert(object.count == 1);
    unregisterElementOwnerCandidate(0,handle,entries,2);
    assert(object.count == 0);
    unregisterElementOwnerCandidate(0,handle,entries,2);
    assert(object.count == 0xffffffffu); // Original does not clamp underflow.
    registerElementOwnerCandidate(0,handle,entries,2);
    registerElementOwnerCandidate(0,handle,entries,2);
    assert(object.count == 1);
    entries[1].generationAndFlags |= 0x80000000u;
    unregisterElementOwnerCandidate(0,handle,entries,2);
    registerElementOwnerCandidate(17,handle,entries,2);
    assert(object.count == 1);
    entries[1].generationAndFlags = 0x17ff; // Unexpected low31 bits reject.
    registerElementOwnerCandidate(17,handle,entries,2);
    assert(object.count == 1);
    entries[1] = {0x7ff,nullptr};
    registerElementOwnerCandidate(17,handle,entries,2);
    assert(object.count == 1);
    ElementOwnerCounterCandidate oldOwner{1},newOwner{0};
    ElementOwnerPoolEntryCandidate transitionEntries[2]{{4,&oldOwner},{5,&newOwner}};
    std::uint32_t ownerHandle = 0x400000u;
    transitionElementOwnerPhase3Candidate(123,ownerHandle,0x500001u,transitionEntries,2);
    assert(ownerHandle == 0x500001u && oldOwner.count == 0 && newOwner.count == 1);
    assert(elementOwnerCountCandidate(ownerHandle,transitionEntries,2)==1);
    transitionElementOwnerPhase3Candidate(123,ownerHandle,ownerHandle,transitionEntries,2);
    assert(newOwner.count == 1);
    transitionElementOwnerPhase3Candidate(123,ownerHandle,0xffffffffu,transitionEntries,2);
    assert(ownerHandle == 0xffffffffu && newOwner.count == 0);
    transitionElementOwnerPhase3Candidate(123,ownerHandle,0x600001u,transitionEntries,2);
    assert(ownerHandle == 0x600001u && newOwner.count == 0); // Stores stale handle too.
    std::puts("x64=1 owner_pool_boundaries=passed phase3_transition=passed graph_walk=passed original_runtime_parity=unverified");
}
