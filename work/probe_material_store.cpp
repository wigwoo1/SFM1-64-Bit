#include <cstdlib>
#include <new>
#include <cstdint>
// Test-only delete observation: tracks explicitly registered source roots and
// heap calls, never scans the process or reads a freed object.
struct Watched {void*p=nullptr;char kind=0;unsigned id=0;bool live=false;};
struct Freed {char kind=0;unsigned id=0;};
static Watched watched[20000];static Freed freed[20000];static size_t watches=0,frees=0;
static size_t heapAllocations=0,heapReleases=0;
static int64_t failAllocationAfter=-1;
static void observeFree(void*p)noexcept{for(size_t i=watches;i;i--)if(watched[i-1].p==p&&watched[i-1].live){watched[i-1].live=false;if(frees==20000)std::abort();freed[frees++]={watched[i-1].kind,watched[i-1].id};return;}}
void*operator new(size_t n){if(failAllocationAfter>=0&&failAllocationAfter--==0)throw std::bad_alloc();auto*p=std::malloc(n?n:1);if(!p)throw std::bad_alloc();++heapAllocations;return p;}
void operator delete(void*p)noexcept{if(p)++heapReleases;observeFree(p);std::free(p);}
void operator delete(void*p,size_t)noexcept{::operator delete(p);}
void*operator new[](size_t n){return ::operator new(n);}
void operator delete[](void*p)noexcept{::operator delete(p);}
void operator delete[](void*p,size_t)noexcept{::operator delete(p);}
#ifdef SFM64_NATIVE_MATERIAL_STORE
#define SFM64_QUEUED_MATERIAL_LIFETIME_FIXTURE_ONLY
#include "probe_queued_material_lifetime.cpp"
#else
#include "material_object_store_candidate.h"
#endif
#include <iomanip>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
static void requireStore(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static void watch(void*p,char kind,unsigned id){requireStore(p&&watches<20000,"Watch bound");watched[watches++]={p,kind,id,true};}
static bool watchedLive(void*p){for(size_t i=watches;i;i--)if(watched[i-1].p==p)return watched[i-1].live;return false;}
static SourceMaterialPatchNodeCandidate sourceTree(){SourceMaterialPatchNodeCandidate n;n.key="UnlitGeneric";SourceMaterialPatchNodeCandidate v;v.key="$basetexture";v.kind=1;v.text="fixture/original";n.children.push_back(v);SourceMaterialPatchNodeCandidate block;block.key="nested";v.key="value";v.text="nested/value";block.children.push_back(v);n.children.push_back(block);return n;}
struct LogicalMaterial {unsigned id=0,sourceTag=0;void*raw=nullptr;void*source=nullptr;bool live=true;};
struct LogicalCall {void*p=nullptr;unsigned id=0,target=0,op=0;};
#ifdef SFM64_NATIVE_MATERIAL_STORE
struct StoreNativeFixture:QueuedMaterialLifetimeFixture{
    using QueuedMaterialLifetimeFixture::QueuedMaterialLifetimeFixture;
    static void __fastcall trackedHeapFree(void*a,void*b,void*p){observeFree(p);MaterialFallbackNativeFixture::heapFree(a,b,p);}
    static void __fastcall trackedKVFree(void*a,void*b,void*p){observeFree(p);MaterialFallbackNativeFixture::kvFree(a,b,p);}
    void beginStore(){start();allocators[5]=reinterpret_cast<uintptr_t>(&trackedHeapFree);kvMethods[2]=reinterpret_cast<uintptr_t>(&trackedKVFree);}
};
#endif
struct StoreAdapter {
    std::map<unsigned,LogicalMaterial>materials;std::vector<LogicalCall>callHistory;
    std::vector<unsigned>destructionOrder;unsigned nextId=1;void*held=nullptr;
#ifdef SFM64_NATIVE_MATERIAL_STORE
    StoreNativeFixture f;
    StoreAdapter(char**argv):f(argv[1],argv[2],argv[3]){f.beginStore();}
    unsigned logical(void*p)const{for(const auto&e:materials)if(e.second.live&&e.second.raw==p)return e.first;throw std::runtime_error("Unknown live material");}
    void update(){for(size_t i=destructionOrder.size();i<f.destroyed.size();++i){auto id=logical(f.rawAt(f.destroyed[i]));materials.at(id).live=false;destructionOrder.push_back(id);}}
    unsigned create(unsigned mode,bool supplied,unsigned tag,const std::string&name,const std::string&group){
        void*source=supplied?f.build(sourceTree()):nullptr;if(source)watch(source,'S',tag);void*raw=nullptr;
        if(mode==0){using F=void*(__cdecl*)(const char*,const char*,void*);raw=reinterpret_cast<F>(0x10006cc0)(name.c_str(),group.c_str(),source);}
        else if(mode==1){using F=void*(__thiscall*)(void*,const char*,void*);raw=static_cast<uint8_t*>(reinterpret_cast<F>(0x1000b790)(f.system(),name.c_str(),source))-0x80;}
        else{using F=void*(__thiscall*)(void*,const char*,const char*,void*);raw=static_cast<uint8_t*>(reinterpret_cast<F>(0x10011860)(f.system(),name.c_str(),group.c_str(),source))-0x80;}
        for(const auto&e:materials)if(e.second.live&&e.second.raw==raw){requireStore(!source||!watchedLive(source),"Helper destroys unused source");return e.first;}
        auto id=nextId++;materials.emplace(id,LogicalMaterial{id,supplied?tag:0,raw,source,true});requireStore(f.read(raw,0x7c)==reinterpret_cast<uintptr_t>(source),"Exact native source adoption");return id;
    }
    void change(unsigned id,const std::string&value){auto&m=materials.at(id);requireStore(m.live&&m.source,"Native mutation target");using F=void(__thiscall*)(void*,const char*,const char*);reinterpret_cast<F>(0x10070db0)(m.source,"$basetexture",value.c_str());}
    void reference(unsigned id,bool increment){using F=void(__thiscall*)(void*);reinterpret_cast<F>(increment?0x10005dc0:0x1002dd90)(materials.at(id).raw);}
    void remove(unsigned id,unsigned op){auto*p=materials.at(id).raw;if(op==0)f.removeOnly(p);else if(op==1)f.deleteIfUnreferenced(p);else{using F=void(__thiscall*)(void*,void*);reinterpret_cast<F>(0x10009b80)(f.dictionary(),p);}update();}
    unsigned find(const std::string&name,bool source){auto slot=f.find(name,source);return slot==f.capacity?0:logical(f.rawAt(slot));}
    std::vector<unsigned>cache(){std::vector<unsigned>v;for(auto id:f.enumerate())v.push_back(logical(f.rawAt(id)));return v;}
    std::vector<void*>calls(){std::vector<void*>v;for(auto n=f.read(f.queue());n;n=f.read(reinterpret_cast<void*>(n)))v.push_back(reinterpret_cast<void*>(f.read(reinterpret_cast<void*>(n),4)));return v;}
    uint32_t refs(void*p){return f.read(p,12);}
    unsigned addRef(void*p){return f.reference(p,0);}
    unsigned release(void*p){return f.reference(p,1);}
    unsigned warnings(){return f.warnings;}
    void forward(unsigned id,unsigned op,bool queued){f.enabled=queued;f.forward(f.id(materials.at(id).raw),op?51:13);update();registerCall(id,op);}
    void drain(){f.drain();update();}
    void recordMaterial(std::ostream&out,const LogicalMaterial&m){
        using Name=const char*(__thiscall*)(void*);using Identity=void*(__thiscall*)(void*);auto*p=m.raw;auto*forward=static_cast<uint8_t*>(p)+0x80;
        auto*source=reinterpret_cast<void*>(f.read(p,0x7c));requireStore(source==m.source,"Adopted native source remains identical");
        out<<std::quoted(reinterpret_cast<Name>(0x10004190)(p))<<' '<<std::quoted(reinterpret_cast<Name>(0x100041a0)(p))<<' '<<int32_t(f.read(p,0x1c))<<' '<<(f.read(p,0x20)&0xffff)<<' '<<m.sourceTag<<' ';
        std::string value,nested;if(source){auto n=f.extract(source);value=n.children.at(0).text;nested=n.children.at(1).children.at(0).text;requireStore(watchedLive(source),"Native owned source alive");}
        out<<std::quoted(value)<<' '<<std::quoted(nested)<<' '<<(reinterpret_cast<Identity>(0x100067b0)(p)==forward&&reinterpret_cast<Identity>(0x100043d0)(forward)==p)<<' '<<(f.read(p,0x14)==0&&f.read(p,0x24)==0&&(f.read(p,0x20)>>16)==0);
        for(size_t n:{0x6cu,0x70u,0x74u})out<<' '<<f.read(p,n);
    }
    void finish(){requireStore(!held,"Held native functor");f.stop();f.finishLifetime();std::cout<<"native_allocations="<<f.allocations<<" releases="<<f.releases<<'\n';}
#else
    MaterialObjectStoreCandidate store;
    explicit StoreAdapter(char**){static_assert(sizeof(void*)==8,"Owned store is x64");}
    void update(){for(size_t i=0;i<frees;++i)if(freed[i].kind=='D'&&materials.at(freed[i].id).live){materials.at(freed[i].id).live=false;destructionOrder.push_back(freed[i].id);}}
    unsigned create(unsigned mode,bool supplied,unsigned tag,const std::string&name,const std::string&group){
        MaterialObjectStoreCandidate::Source source;if(supplied){source=std::make_unique<SourceMaterialPatchNodeCandidate>(sourceTree());watch(source.get(),'S',tag);}auto*p=source.get();uintptr_t id=0;
        bool ok=mode==0?store.createRaw(name,group,source,id):mode==1?store.createMaterial(name,source,id):store.createOrReuse(name,group,source,id);requireStore(ok&&!source,"Owned create/adopt");
        if(materials.count(unsigned(id))){requireStore(!p||!watchedLive(p),"Helper destroys unused owned source");return unsigned(id);}
        auto*observed=const_cast<MaterialObjectStoreCandidate::Object*>(store.get(id)); // Observation only; ownership remains private.
        requireStore(id==nextId++,"Monotonic owned identity");materials.emplace(unsigned(id),LogicalMaterial{unsigned(id),supplied?tag:0,observed,p,true});watch(observed,'D',unsigned(id));requireStore(store.get(id)->source.get()==p,"Exact owned source adoption");return unsigned(id);
    }
    void change(unsigned id,const std::string&value){auto&m=materials.at(id);requireStore(m.live&&m.source,"Owned mutation target");static_cast<SourceMaterialPatchNodeCandidate*>(m.source)->children.at(0).text=value;}
    void reference(unsigned id,bool increment){requireStore(increment?store.increment(id):store.decrement(id),"Owned reference target");}
    void remove(unsigned id,unsigned op){if(op==0)store.removeOnly(id);else if(op==1)store.deleteIfUnreferenced(id);else store.removeAndDelete(id);update();}
    unsigned find(const std::string&name,bool source){return unsigned(store.find(name,source));}
    std::vector<unsigned>cache(){std::vector<unsigned>v;for(auto id:store.cached())v.push_back(unsigned(id));return v;}
    std::vector<void*>calls(){std::vector<void*>v;for(auto*n=store.queueState().head;n;n=n->next)v.push_back(n->call);return v;}
    uint32_t refs(void*p){return static_cast<StudioDecalQueuedCallCandidate*>(p)->references.load();}
    unsigned addRef(void*p){return static_cast<StudioDecalQueuedCallCandidate*>(p)->addReference();}
    unsigned release(void*p){return static_cast<StudioDecalQueuedCallCandidate*>(p)->releaseReference([](auto*p){delete p;});}
    unsigned warnings(){return unsigned(store.statistics().referenceWarnings);}
    void forward(unsigned id,unsigned op,bool queued){requireStore(store.forward(id,op?MaterialObjectStoreCandidate::Operation::deleteIfUnreferenced:MaterialObjectStoreCandidate::Operation::decrement,queued),"Owned forward");update();registerCall(id,op);}
    void drain(){store.drain();update();}
    void recordMaterial(std::ostream&out,const LogicalMaterial&m){
        auto*p=store.get(m.id);requireStore(p&&p->source.get()==m.source,"Adopted owned source remains identical");
        out<<std::quoted(p->name)<<' '<<std::quoted(p->group)<<' '<<p->references.load()<<' '<<p->flags<<' '<<m.sourceTag<<' ';
        std::string value,nested;if(p->source){value=p->source->children.at(0).text;nested=p->source->children.at(1).children.at(0).text;requireStore(watchedLive(p->source.get()),"Owned source alive");}
        out<<std::quoted(value)<<' '<<std::quoted(nested)<<' '<<(store.forwarding(m.id)==&p->forwarding&&p->forwarding.raw==p)<<' '<<(!p->parameterState&&p->parameterCount==0);
        for(auto f:p->initialValues){uint32_t bits;std::memcpy(&bits,&f,4);out<<' '<<bits;}
    }
    void finish(){requireStore(!held&&store.size()==0&&store.queuedCount()==0&&store.valid(),"Owned final cleanup");}
#endif
    void registerCall(unsigned target,unsigned op){for(auto*p:calls()){
        bool known=false;for(auto i=callHistory.rbegin();i!=callHistory.rend();++i)if(i->p==p){known=watchedLive(p);break;}
        if(!known){auto id=unsigned(callHistory.size()+1);callHistory.push_back({p,id,target,op});watch(p,'F',id);}
    }}
    void hold(unsigned index){auto v=calls();requireStore(!held&&index<v.size(),"Hold queue index");held=v[index];requireStore(addRef(held)==2,"Heap functor addref");}
    void releaseHeld(){requireStore(held&&watchedLive(held),"Live held call");requireStore(release(held)==0,"Held call final release");held=nullptr;}
    void record(std::ostream&out,unsigned step,char command,unsigned result){
        out<<step<<' '<<command<<' '<<result<<" |";for(const auto&e:materials)if(e.second.live){out<<" M "<<e.first<<' ';recordMaterial(out,e.second);}
        out<<" | C";for(auto id:cache())out<<' '<<id;out<<" | Q";for(auto*p:calls()){
            const LogicalCall*c=nullptr;for(auto i=callHistory.rbegin();i!=callHistory.rend();++i)if(i->p==p){c=&*i;break;}requireStore(c,"Queued identity");
#ifdef SFM64_NATIVE_MATERIAL_STORE
            requireStore(f.read(p,16)==(c->op?0x10021850u:0x100218a0u)&&f.read(p,20)==reinterpret_cast<uintptr_t>(materials.at(c->target).raw),"Native copied call payload");
#else
            auto*call=static_cast<StudioDecalQueuedCallCandidate*>(p);requireStore(call->argument==c->target&&call->owner==reinterpret_cast<uintptr_t>(&store),"Owned copied call payload");
#endif
            out<<' '<<c->id<<':'<<c->target<<':'<<c->op<<':'<<refs(p);
        }
        out<<" | D";for(auto id:destructionOrder)out<<' '<<id;out<<" | F";for(size_t i=0;i<frees;++i)if(freed[i].kind!='D')out<<' '<<freed[i].kind<<freed[i].id;
        out<<" | H "<<(held?1:0);if(held)out<<':'<<refs(held)<<':'<<watchedLive(held);out<<" | W "<<warnings()<<'\n';
    }
};

#ifndef SFM64_NATIVE_MATERIAL_STORE
static int storeSafety();
#endif
int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_MATERIAL_STORE
    if(argc==2&&std::string(argv[1])=="--test")return storeSafety();
#endif
    requireStore(argc==6,"material-or-absent libraries-or-absent workspace-mount query fresh-output");requireStore(!std::ifstream(argv[5]).good(),"Fresh output required");
    std::ifstream in(argv[4]);std::ofstream out(argv[5]);requireStore(bool(in)&&bool(out),"Trace IO");StoreAdapter a(argv);unsigned steps=0;std::string line;
    while(std::getline(in,line)){std::istringstream q(line);char command;unsigned id=0,op=0,flag=0,result=0;std::string text,group;q>>command;
        if(command=='C'){unsigned tag;q>>op>>flag>>tag>>std::quoted(text)>>std::quoted(group);result=a.create(op,flag!=0,tag,text,group);}
        else if(command=='F'){q>>flag>>std::quoted(text);result=a.find(text,flag!=0);}
        else if(command=='M'){q>>id>>std::quoted(text);a.change(id,text);}
        else if(command=='Q'){q>>id>>op>>flag;a.forward(id,op,flag!=0);}
        else if(command=='H'){q>>id;a.hold(id);}
        else if(command=='L')a.releaseHeld();else if(command=='N')a.drain();
        else{q>>id;if(command=='I'||command=='R')a.reference(id,command=='I');else if(command=='E'||command=='D'||command=='X')a.remove(id,command=='E'?0:command=='D'?1:2);else requireStore(false,"Query command");}
        requireStore(bool(q),"Query parse");q>>std::ws;requireStore(q.eof(),"Query tail");a.record(out,steps++,command,result);
    }
    requireStore(in.eof()&&steps,"Complete queries");a.finish();for(size_t i=0;i<watches;++i)requireStore(!watched[i].live,"Every watched source/functor destroyed");out.flush();requireStore(bool(out),"Complete trace");
    size_t rootsAndCalls=0;for(size_t i=0;i<watches;++i)if(watched[i].kind!='D')++rootsAndCalls;
    std::cout<<"PASS material_store_operations="<<steps<<" roots_and_calls_destroyed="<<rootsAndCalls<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

#ifndef SFM64_NATIVE_MATERIAL_STORE
static int storeSafety(){
    using S=MaterialObjectStoreCandidate;using A=S::Allocation;using Op=S::Operation;unsigned checks=0;auto check=[&](bool b){requireStore(b,"Owned material-store safety");++checks;};
    S s;auto make=[](){return std::make_unique<SourceMaterialPatchNodeCandidate>(sourceTree());};uintptr_t id=777;auto source=make();auto*alias=source.get();
    for(auto stage:{A::symbols,A::object,A::registry}){
        const auto balance=heapAllocations-heapReleases;
        check(!s.createMaterial("safety/untouched",source,id,[&](A kind,size_t){return kind!=stage;})&&source.get()==alias&&id==777&&s.size()==0&&s.symbolCount()==0&&s.cached().empty());
        check(!s.createMaterial("safety/untouched",source,id,[&](A kind,size_t)->bool{if(kind==stage)throw std::bad_alloc();return true;})&&source.get()==alias&&id==777&&s.size()==0&&s.symbolCount()==0);
        check(heapAllocations-heapReleases==balance);
    }
    check(s.createMaterial("safety/first",source,id)&&!source&&s.get(id)->source.get()==alias&&s.get(id)->references.load()==1);
    auto original=id;auto*raw=s.get(id);auto*forward=s.forwarding(id);check(raw&&forward&&forward->raw==raw);
    for(auto stage:{A::functor,A::append}){
        const auto balance=heapAllocations-heapReleases;
        check(!s.forward(id,Op::decrement,true,[&](A k,size_t){return k!=stage;})&&raw->references.load()==1&&s.queuedCount()==0);
        check(!s.forward(id,Op::decrement,true,[&](A k,size_t)->bool{if(k==stage)throw std::bad_alloc();return true;})&&raw->references.load()==1&&s.queuedCount()==0);
        check(heapAllocations-heapReleases==balance);
    }
    check(s.forward(id,Op::decrement,true)&&raw->references.load()==1);check(s.removeAndDelete(id));source=make();check(s.createMaterial("safety/recycled",source,id)&&id!=original);
    s.drain();check(s.statistics().staleCalls==1&&s.get(id)->references.load()==1&&!s.increment(original)&&!s.decrement(original)&&!s.removeAndDelete(original)&&!s.forward(original,Op::decrement,true));
    s.decrement(id);check(s.deleteIfUnreferenced(id));
    // Force real tree growth refusal after256 live entries; source and complete
    // public state remain unchanged and the next successful ID is not consumed.
    std::vector<uintptr_t>ids;for(unsigned i=0;i<256;++i){source=make();check(s.createMaterial("growth/"+std::to_string(i),source,id));ids.push_back(id);}
    source=make();alias=source.get();const auto names=s.symbolCount(),count=s.size();const auto cached=s.cached();id=777;
    for(bool throwing:{false,true}){const auto balance=heapAllocations-heapReleases;check(!s.createMaterial("growth/fail",source,id,[&](A k,size_t)->bool{if(k==A::tree){if(throwing)throw std::bad_alloc();return false;}return true;})&&source.get()==alias&&id==777&&s.symbolCount()==names&&s.size()==count&&s.cached()==cached&&s.valid());check(heapAllocations-heapReleases==balance);}
    check(s.createMaterial("growth/fail",source,id)&&s.get(id)->source.get()==alias);ids.push_back(id);for(auto n:ids){s.decrement(n);check(s.deleteIfUnreferenced(n));}
    // Fail every actual C++ heap allocation in one complete creation until the
    // first fully successful attempt. Hooks above are separate policy checks.
    const std::string allocationName(200,'n');unsigned allocationFailures=0;bool completed=false;
    for(int64_t nth=0;nth<128;++nth){
        source=make();alias=source.get();id=777;const auto symbolCount=s.symbolCount(),balance=heapAllocations-heapReleases;
        failAllocationAfter=nth;bool ok=s.createMaterial(allocationName,source,id);failAllocationAfter=-1;
        if(ok){check(!source&&s.get(id)->source.get()==alias);s.decrement(id);check(s.deleteIfUnreferenced(id));completed=true;break;}
        ++allocationFailures;check(source.get()==alias&&id==777&&s.size()==0&&s.symbolCount()==symbolCount&&s.cached().empty()&&heapAllocations-heapReleases==balance);
    }
    check(completed&&allocationFailures>=5);
    // A held call can outlive even the store after its one queued invocation;
    // final Release only destroys the call and never dereferences its old owner.
    StudioDecalQueuedCallCandidate*retained=nullptr;void*watchedSource=nullptr;
    {S lifecycle;source=make();watchedSource=source.get();watch(watchedSource,'S',90001);check(lifecycle.createMaterial("scope/cleanup",source,id));check(lifecycle.forward(id,Op::decrement,true)&&lifecycle.forward(id,Op::deleteIfUnreferenced,true));retained=lifecycle.queueState().tail->call;watch(retained,'F',90002);check(retained->addReference()==2);}
    check(!watchedLive(watchedSource)&&watchedLive(retained)&&retained->references.load()==1);check(retained->releaseReference([](auto*p){delete p;})==0&&!watchedLive(retained));
    // Real existing queue-buffer exhaustion, not only a synthetic append hook.
    {S boundedQueueStore(0x10000,0x10000);source=make();check(boundedQueueStore.createMaterial("queue/full",source,id));size_t accepted=0;while(boundedQueueStore.forward(id,Op::decrement,true)){++accepted;check(accepted<=4096);}check(accepted==4096&&boundedQueueStore.queuedCount()==4096&&boundedQueueStore.get(id)->references.load()==1);const auto balance=heapAllocations-heapReleases;check(!boundedQueueStore.forward(id,Op::decrement,true)&&heapAllocations-heapReleases==balance&&boundedQueueStore.queuedCount()==4096);boundedQueueStore.drain();check(boundedQueueStore.get(id)->references.load()==-4095&&boundedQueueStore.queuedCount()==0&&boundedQueueStore.deleteIfUnreferenced(id));}
    source=make();alias=source.get();id=777;check(!s.createMaterial(std::string("bad\0name",8),source,id)&&source.get()==alias&&id==777);check(!s.createMaterial(std::string(4097,'a'),source,id)&&source.get()==alias);source.reset();
    check(s.size()==0&&s.cached().empty()&&s.queuedCount()==0&&s.valid());
    std::cout<<"PASS owned_store_safety_checks="<<checks<<" real_queue_capacity=4096 stale_target_refused=1 actual_allocation_failures="<<allocationFailures<<"\n";return 0;
}
#endif
