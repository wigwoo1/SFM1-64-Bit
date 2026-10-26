#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <new>
#include <vector>

// Independent primary material dictionary storage. It owns tree nodes, never
// the material payload. Payload width follows the host; symbols/links retain
// the original uint16 domain. Object/reference/source/queue ownership belongs
// to the future material layer, not this container.
class MaterialDictionaryTreeCandidate{
public:
    using Index=uint16_t;
    static constexpr Index none=0xffff;
    struct Entry{uintptr_t material=0;uint16_t symbol=none;bool suppliedSource=false;};
    struct Node{Index left=none,right=none,parent=none;uint16_t color=1;Entry entry;};
    using Permit=std::function<bool(size_t)>;
private:
    std::unique_ptr<Node[]>nodes;
    Index rootIndex=none,freeIndex=none;
    size_t allocated=0,used=0,storage=0;
    uint16_t nilColor=1;
    static bool less(const Entry&a,const Entry&b){return a.suppliedSource!=b.suppliedSource?a.suppliedSource:a.symbol<b.symbol;}
    Index left(Index i)const{return i==none?none:nodes[i].left;}
    Index right(Index i)const{return i==none?none:nodes[i].right;}
    Index parent(Index i)const{return i==none?none:nodes[i].parent;}
    uint16_t color(Index i)const{return i==none?nilColor:nodes[i].color;}
    // Original nil recoloring is guarded (e.g.10009006..10009015). Nil stays
    // black even after the observed nil-child removal-repair skip.
    void paint(Index i,uint16_t c){if(i!=none)nodes[i].color=c;}
    bool reserve(size_t n,const Permit&permit){
        try{if(permit&&!permit(n*sizeof(Node)))return false;auto next=std::make_unique<Node[]>(n);std::copy_n(nodes.get(),allocated,next.get());nodes=std::move(next);storage=n;return true;}catch(const std::bad_alloc&){return false;}
    }
    void rotateLeft(Index x){
        const Index y=right(x);nodes[x].right=left(y);if(left(y)!=none)nodes[left(y)].parent=x;
        nodes[y].parent=parent(x);if(parent(x)==none)rootIndex=y;else if(x==left(parent(x)))nodes[parent(x)].left=y;else nodes[parent(x)].right=y;
        nodes[y].left=x;nodes[x].parent=y;
    }
    void rotateRight(Index x){
        const Index y=left(x);nodes[x].left=right(y);if(right(y)!=none)nodes[right(y)].parent=x;
        nodes[y].parent=parent(x);if(parent(x)==none)rootIndex=y;else if(x==right(parent(x)))nodes[parent(x)].right=y;else nodes[parent(x)].left=y;
        nodes[y].right=x;nodes[x].parent=y;
    }
    void insertFix(Index x){
        while(x!=rootIndex&&color(parent(x))==0){
            if(parent(x)==left(parent(parent(x)))){
                const Index uncle=right(parent(parent(x)));
                if(color(uncle)==0){paint(parent(x),1);paint(uncle,1);paint(parent(parent(x)),0);x=parent(parent(x));}
                else{if(x==right(parent(x))){x=parent(x);rotateLeft(x);}paint(parent(x),1);paint(parent(parent(x)),0);rotateRight(parent(parent(x)));}
            }else{
                const Index uncle=left(parent(parent(x)));
                if(color(uncle)==0){paint(parent(x),1);paint(uncle,1);paint(parent(parent(x)),0);x=parent(parent(x));}
                else{if(x==left(parent(x))){x=parent(x);rotateRight(x);}paint(parent(x),1);paint(parent(parent(x)),0);rotateLeft(parent(parent(x)));}
            }
        }
        paint(rootIndex,1);
    }
    void removeFix(Index x){
        while(x!=rootIndex&&color(x)==1){
            if(x==left(parent(x))){
                Index sibling=right(parent(x));
                if(color(sibling)==0){paint(sibling,1);paint(parent(x),0);rotateLeft(parent(x));sibling=right(parent(x));}
                if(color(left(sibling))==1&&color(right(sibling))==1){paint(sibling,0);x=parent(x);}
                else{if(color(right(sibling))==1){paint(left(sibling),1);paint(sibling,0);rotateRight(sibling);sibling=right(parent(x));}
                    paint(sibling,color(parent(x)));paint(parent(x),1);paint(right(sibling),1);rotateLeft(parent(x));x=rootIndex;}
            }else{
                Index sibling=left(parent(x));
                if(color(sibling)==0){paint(sibling,1);paint(parent(x),0);rotateRight(parent(x));sibling=left(parent(x));}
                if(color(right(sibling))==1&&color(left(sibling))==1){paint(sibling,0);x=parent(x);}
                else{if(color(left(sibling))==1){paint(right(sibling),1);paint(sibling,0);rotateLeft(sibling);sibling=left(parent(x));}
                    paint(sibling,color(parent(x)));paint(parent(x),1);paint(left(sibling),1);rotateRight(parent(x));x=rootIndex;}
            }
        }
        paint(x,1);
    }
public:
    explicit MaterialDictionaryTreeCandidate(){if(!reserve(256,{}))throw std::bad_alloc();}
    MaterialDictionaryTreeCandidate(const MaterialDictionaryTreeCandidate&)=delete;
    MaterialDictionaryTreeCandidate&operator=(const MaterialDictionaryTreeCandidate&)=delete;
    size_t size()const{return used;}
    size_t capacity()const{return storage;}
    size_t allocatedSlots()const{return allocated;}
    Index root()const{return rootIndex;}
    Index freeHead()const{return freeIndex;}
    uint16_t sentinelColor()const{return nilColor;}
    const Node*slot(Index i)const{return i<allocated?&nodes[i]:nullptr;}
    bool live(Index i)const{return i<allocated&&nodes[i].left!=i;}
    Index first()const{Index x=rootIndex;if(x!=none)while(left(x)!=none)x=left(x);return x;}
    Index next(Index x)const{
        if(!live(x))return none;if(right(x)!=none){x=right(x);while(left(x)!=none)x=left(x);return x;}
        Index p=parent(x);while(p!=none&&x==right(p)){x=p;p=parent(p);}return p;
    }
    Index find(uint16_t symbol,bool source)const{
        Entry key{0,symbol,source};Index x=rootIndex;
        while(x!=none){if(less(key,nodes[x].entry))x=left(x);else if(less(nodes[x].entry,key))x=right(x);else return x;}return none;
    }
    bool insert(Entry entry,Index&output,const Permit&permit={}){
        Index p=none,x=rootIndex;bool isLeft=false;
        while(x!=none){p=x;isLeft=less(entry,nodes[x].entry);x=isLeft?left(x):right(x);}
        Index id=freeIndex;
        if(id==none){
            if(allocated==none)return false;
            if(allocated==storage&&!reserve(std::min<size_t>(none,storage?storage*2:2),permit))return false;
            id=static_cast<Index>(allocated++);
        }else freeIndex=right(id);
        nodes[id]={none,none,p,0,entry};if(p==none)rootIndex=id;else if(isLeft)nodes[p].left=id;else nodes[p].right=id;
        insertFix(id);++used;output=id;return true;
    }
    bool erase(Index z){
        if(!live(z))return false;
        Index y=z;if(left(y)!=none&&right(y)!=none){y=right(y);while(left(y)!=none)y=left(y);}
        const Index child=left(y)!=none?left(y):right(y),oldParent=parent(y);const uint16_t removedColor=color(y);
        if(child!=none)nodes[child].parent=oldParent;
        if(oldParent==none)rootIndex=child;else if(y==left(oldParent))nodes[oldParent].left=child;else nodes[oldParent].right=child;
        if(y!=z){
            nodes[y].parent=parent(z);nodes[y].right=right(z);nodes[y].left=left(z);
            if(parent(z)==none)rootIndex=y;else if(z==left(parent(z)))nodes[parent(z)].left=y;else nodes[parent(z)].right=y;
            if(left(y)!=none)nodes[left(y)].parent=y;if(right(y)!=none)nodes[right(y)].parent=y;nodes[y].color=color(z);
        }
        // Original100099F1..10009A09 deliberately skips nil-child repair.
        // Preserve this observed variant rather than assuming CLRS nil fixup.
        if(child!=none&&removedColor==1)removeFix(child);
        nodes[z].left=z;nodes[z].right=freeIndex;freeIndex=z;--used;return true;
    }
    Index eraseMaterial(uintptr_t value){for(Index i=first();i!=none;i=next(i))if(nodes[i].entry.material==value){erase(i);return i;}return none;}
    void purge(){nodes.reset();rootIndex=freeIndex=none;allocated=used=storage=0;}
    // Structural/order/free-list checks do not invent a stronger post-delete
    // red/black balance invariant than the original variant actually promises.
    bool valid()const{
        if(allocated>storage||allocated>none||used>allocated)return false;
        std::vector<uint8_t>seen(allocated);std::vector<Index>todo;if(rootIndex!=none){if(!live(rootIndex)||parent(rootIndex)!=none)return false;todo.push_back(rootIndex);}
        size_t count=0;while(!todo.empty()){
            const auto i=todo.back();todo.pop_back();if(!live(i)||seen[i]++||color(i)>1)return false;++count;
            for(auto c:{left(i),right(i)})if(c!=none){if(!live(c)||parent(c)!=i)return false;todo.push_back(c);}
        }
        if(count!=used)return false;Index previous=none;count=0;
        for(Index i=first();i!=none;i=next(i)){if(++count>used||(previous!=none&&less(nodes[i].entry,nodes[previous].entry)))return false;previous=i;}
        if(count!=used)return false;count=0;
        for(Index i=freeIndex;i!=none;i=right(i)){if(i>=allocated||live(i)||seen[i]++||++count>allocated)return false;}
        return count+used==allocated;
    }
};
