#define SFM64_MATERIAL_DICTIONARY_FIXTURE_ONLY
#include "probe_material_dictionary.cpp"

struct QueuedMaterialLifetimeFixture:MaterialDictionaryFixture{
    std::array<uint8_t,0xb8+32>queueStorage{};
    bool enabled=true,failFunctor=false;unsigned failedAllocations=0;
    static QueuedMaterialLifetimeFixture&self(){return *static_cast<QueuedMaterialLifetimeFixture*>(active);}
    static void* __fastcall context(void*object,void*){auto&f=self();if(object!=f.system())ExitProcess(230);++f.contextCalls;return &f.queueObject;}
    static void* __fastcall callQueue(void*object,void*){auto&f=self();if(object!=&f.queueObject)ExitProcess(231);++f.queueCalls;return f.enabled?f.queue()+0xb4:nullptr;}
    static void* __fastcall allocateFunctor(void*a,void*b,uint32_t size){auto&f=self();if(f.failFunctor&&size==24){++f.failedAllocations;return nullptr;}return MaterialTypedDispatchNativeFixture::allocate(a,b,size);}
    uint8_t*queue(){return queueStorage.data()+16;}
    explicit QueuedMaterialLifetimeFixture(const char*p,const char*d,const char*m):MaterialDictionaryFixture(p,d,m){
        lifetimeMethods[103]=reinterpret_cast<uintptr_t>(&context);queueMethods[143]=reinterpret_cast<uintptr_t>(&callQueue);
        word(0x1255cc,reinterpret_cast<uintptr_t>(system()));allocators[1]=reinterpret_cast<uintptr_t>(&allocateFunctor);
    }
    void start(){
        initialize();enabled=true;failFunctor=false;contextCalls=queueCalls=failedAllocations=0;
        queueStorage.fill(0xa5);std::memset(queue(),0,8);const uint32_t table=0x10096854;std::memcpy(queue()+0xb4,&table,4);
        using Construct=void*(__thiscall*)(void*);reinterpret_cast<Construct>(0x10073950)(queue()+8);
        using Init=bool(__thiscall*)(void*,uint32_t,uint32_t,uint32_t,uint32_t);
        if(!reinterpret_cast<Init>(0x100739c0)(queue()+8,0x100000,0x10000,0x10000,4))throw std::runtime_error("Original call-queue buffer init");
    }
    unsigned create(bool sourced,unsigned refs,const std::string&name){
        SourceMaterialPatchNodeCandidate n;n.key="UnlitGeneric";SourceMaterialPatchNodeCandidate v;v.key="$basetexture";v.kind=1;v.text="fixture/queued";n.children.push_back(v);
        void*ownedSource=sourced?build(n):nullptr;using Create=void*(__thiscall*)(void*,const char*,void*);
        auto*raw=static_cast<uint8_t*>(reinterpret_cast<Create>(0x1000b790)(system(),name.c_str(),ownedSource))-0x80;
        using Ref=void(__thiscall*)(void*);for(unsigned i=1;i<refs;++i)reinterpret_cast<Ref>(0x10005dc0)(raw);return id(raw);
    }
    void forward(unsigned id,unsigned slot){auto*raw=rawAt(id);auto*handle=raw+0x80;auto table=*reinterpret_cast<uintptr_t**>(handle);if((slot!=13&&slot!=51)||table[slot]!=(slot==13?0x10007820u:0x100078a0u))throw std::runtime_error("Original forwarding method identity");using Call=void(__thiscall*)(void*);reinterpret_cast<Call>(table[slot])(handle);}
    static unsigned reference(void*p,unsigned slot){using Call=unsigned(__thiscall*)(void*);auto table=*static_cast<uintptr_t**>(p);return reinterpret_cast<Call>(table[slot])(p);}
    bool heapLive(void*p){for(auto i=blocks.rbegin();i!=blocks.rend();++i)if(i->p==p)return i->live;return false;}
    std::vector<void*>queued(){
        std::vector<void*>calls;std::set<uintptr_t>seen;auto next=read(queue());const auto base=read(queue()+8,12),end=read(queue()+8,4);
        while(next){if(next<base||next+8>end||!seen.insert(next).second)throw std::runtime_error("Original queue node extent/cycle");auto*p=reinterpret_cast<void*>(read(reinterpret_cast<void*>(next),4));
            if(!p||!heapLive(p)||read(p)!=0x10096228||read(p,8)!=0x1009621c||read(p,12)!=1||(read(p,16)!=0x100218a0&&read(p,16)!=0x10021850))throw std::runtime_error("Actual heap functor identity/refcount/payload");calls.push_back(p);next=read(reinterpret_cast<void*>(next));}
        return calls;
    }
    void drain(){using Drain=void(__thiscall*)(void*);reinterpret_cast<Drain>(0x100219a0)(queue());if(read(queue())||read(queue(),4)||read(queue()+8)!=read(queue()+8,12))throw std::runtime_error("Original queue drain reset");}
    void stop(){
        if(!queued().empty())throw std::runtime_error("Undrained original calls");cleanupDictionary();cacheGuards();for(unsigned i=0;i<16;++i)if(queueStorage[i]!=0xa5||queueStorage[16+0xb8+i]!=0xa5)throw std::runtime_error("Queue object guard");
        const auto address=read(queue()+8,12);using Release=void(__thiscall*)(void*);reinterpret_cast<Release>(0x10073c70)(queue()+8);MEMORY_BASIC_INFORMATION info{};
        if(read(queue()+8,12)||!VirtualQuery(reinterpret_cast<void*>(address),&info,sizeof(info))||info.State!=MEM_FREE)throw std::runtime_error("Original queue virtual buffer release");verifyCode();
    }
};
static bool expectedNullFailure=false;static unsigned failureSlot=0;
static LONG CALLBACK queuedFault(EXCEPTION_POINTERS*p){
    if(expectedNullFailure&&p->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION&&p->ContextRecord->Eip==0x1000f717&&p->ExceptionRecord->NumberParameters>1&&p->ExceptionRecord->ExceptionInformation[1]==0&&QueuedMaterialLifetimeFixture::self().failedAllocations==1){std::fprintf(stderr,"EXPECTED_UNSAFE_NULL_ENQUEUE slot=%u failed_allocations=1 eip=1000f717 address=0\n",failureSlot);std::fflush(stderr);ExitProcess(238);}
    return lifetimeFault(p);
}
int main(int argc,char**argv){try{
    need(argc==5||argc==6,"material libraries workspace-mount fresh-output [failure-slot]");need(!std::ifstream(argv[4]).good(),"Fresh output required");SetErrorMode(SEM_NOGPFAULTERRORBOX|SEM_FAILCRITICALERRORS);
    QueuedMaterialLifetimeFixture f(argv[1],argv[2],argv[3]);auto observer=AddVectoredExceptionHandler(1,&queuedFault);need(observer!=nullptr,"Fault observer");std::ofstream out(argv[4]);need(bool(out),"Output stream");
    if(argc==6){failureSlot=unsigned(std::stoul(argv[5]));need(failureSlot==13||failureSlot==51,"Failure slot");f.start();auto id=f.create(true,1,"fixture/failure");f.failFunctor=true;expectedNullFailure=true;f.forward(id,failureSlot);throw std::runtime_error("Original null enqueue unexpectedly survived");}
    unsigned checks=0,cases=0,functors=0,held=0;auto check=[&](bool b,const char*s){need(b,s);++checks;};
    for(unsigned scenario=0;scenario<4;++scenario)for(bool sourced:{false,true})for(unsigned refs:{1u,2u,3u,7u}){
        currentCase=cases;f.start();const auto id=f.create(sourced,refs,"fixture/queued"),liveBefore=f.allocations-f.releases;auto*raw=f.rawAt(id);auto snapshot=std::vector<uint8_t>(raw,raw+0xa8);std::vector<void*>calls;
        if(scenario==2){
            f.enabled=false;for(unsigned i=0;i<refs;++i)f.forward(id,13);check(f.read(raw,0x1c)==0&&f.unbindings==0&&f.queued().empty()&&f.allocations-f.releases==liveBefore,"Null queue decrements immediately without functors");f.forward(id,51);check(f.unbindings==1&&f.enumerate().empty(),"Null queue deletes synchronously");f.drain();
        }else{
            if(scenario==1)f.forward(id,51);for(unsigned i=0;i<refs;++i)f.forward(id,13);if(scenario!=1)f.forward(id,51);
            check(std::equal(snapshot.begin(),snapshot.end(),raw)&&f.unbindings==0&&f.enumerate()==std::vector<unsigned>{id},"Enqueue does not change or retain material references");calls=f.queued();check(calls.size()==refs+1,"Exact queued action count");
            for(size_t i=0;i<calls.size();++i){const bool deletion=scenario==1?i==0:i+1==calls.size();check(f.read(calls[i],16)==(deletion?0x10021850u:0x100218a0u)&&f.read(calls[i],20)==reinterpret_cast<uintptr_t>(raw),"FIFO copied method/raw-target pair");}
            void*retained=scenario==3?calls.back():nullptr;if(retained){check(f.reference(retained,0)==2&&f.read(raw,0x1c)==refs,"Functor AddRef does not retain raw target");++held;}
            const auto allocationDelta=f.allocations-f.releases;f.drain();functors+=unsigned(calls.size());
            if(scenario==1){check(f.read(raw,0x1c)==0&&f.unbindings==0&&f.enumerate()==std::vector<unsigned>{id},"FIFO early delete sees positive count then later decrement reaches zero");f.forward(id,51);auto last=f.queued();check(last.size()==1,"Later deletion check queued");f.drain();calls.push_back(last.front());++functors;}
            check(f.unbindings==1&&f.enumerate().empty()&&f.read(raw,0x7c)==0,"Queued delete removes cache/source only during drain");
            if(retained){check(f.heapLive(retained)&&f.read(retained,12)==1&&f.read(retained,20)==reinterpret_cast<uintptr_t>(raw),"Retained functor survives target destruction");const auto before=f.unbindings;check(f.reference(retained,1)==0&&!f.heapLive(retained)&&f.unbindings==before,"Functor final release frees itself without invoking target again");}
            for(auto*p:calls)check(!f.heapLive(p),"Drained functor freed");check(f.allocations-f.releases<allocationDelta,"Drain releases heap resources");
        }
        check(f.contextCalls==f.queueCalls&&f.tokens.empty()&&f.warnings==0,"Context calls/token balance/diagnostics");f.stop();out<<cases<<' '<<scenario<<' '<<sourced<<' '<<refs<<' '<<f.unbindings<<' '<<calls.size()<<'\n';++cases;
    }
    // Mixed targets in one actual FIFO: each early check must observe its
    // still-positive count; decrements precede that target's final check.
    f.start();std::vector<unsigned>ids;for(unsigned i=0;i<64;++i){auto id=f.create(i%2!=0,i%3+1,"fixture/bulk/"+std::to_string(i));ids.push_back(id);f.forward(id,51);for(unsigned n=0;n<i%3+1;++n)f.forward(id,13);f.forward(id,51);}
    auto bulk=f.queued();check(f.unbindings==0&&f.enumerate().size()==64,"Mixed targets retained until drain precondition");f.drain();functors+=unsigned(bulk.size());check(f.destroyed==ids&&f.enumerate().empty()&&f.unbindings==64,"Actual FIFO destruction order across64 targets");for(auto*p:bulk)check(!f.heapLive(p),"Bulk functor cleanup");f.stop();out<<cases<<" B 64 "<<bulk.size()<<'\n';++cases;
    f.finishLifetime();out.flush();check(bool(out),"Complete results");RemoveVectoredExceptionHandler(observer);
    std::cout<<"PASS_REFERENCE_ONLY queued_lifetime_cases="<<cases<<" functors="<<functors<<" retained_functors="<<held<<" independent_checks="<<checks<<" allocations="<<f.allocations<<" releases="<<f.releases<<"\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
