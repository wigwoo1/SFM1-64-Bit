from pathlib import Path
p=Path('work/test_restore_dispatch_native.cpp')
s=p.read_text()
assert 'struct MovingAllocator' not in s, 'One-time fixture migration already applied'
s=s.replace('static bool suppressed[2];', '''using Callback=void(*)(void*,void*,void*,bool);
static bool mutateLists;static unsigned appendedCalls;static void (*mutateList)(unsigned);
static void appended(void*,void*,void*,bool){++appendedCalls;}
struct MovingAllocator{std::size_t size=0;void* allocate(std::size_t n){size=n;return std::malloc(n);}void* reallocate(void* p,std::size_t n){void* q=std::malloc(n);if(!q)return nullptr;std::memcpy(q,p,size);std::free(p);size=n;return q;}void release(void* p){std::free(p);}};
using Registry=NativeManagerCallbackRegistryCandidateT<MovingAllocator,Callback>;
#ifdef _M_IX86
static uint32_t* fixtureOwner;static Callback* oldLists[2];static Callback replacementLists[2][16];
static void relocate(unsigned category){auto* replacement=replacementLists[category];std::memcpy(replacement,oldLists[category],8*sizeof(Callback));replacement[8]=appended;fixtureOwner[(category?0x2ea0:0x2e8c)/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(replacement));fixtureOwner[(category?0x2eac:0x2e98)/4]=9;oldLists[category][1]=appended;}
#else
static Registry* activeLists[2];static void relocate(unsigned category){activeLists[category]->append(appended);}
#endif
static bool suppressed[2];''')
s=s.replace('++calls[category];*flags[category]|=0x40;', '++calls[category];*flags[category]|=0x40;if(mutateLists&&calls[category]==1)mutateList(category);')
s=s.replace('unsigned cases=0;for(bool zeroFirst', 'unsigned cases=0;for(bool mutation:{false,true})for(bool zeroFirst')
s=s.replace('suppressed[0]=suppressFirst;', 'mutateLists=mutation;mutateList=relocate;appendedCalls=0;suppressed[0]=suppressFirst;')
s=s.replace('zeroFirst?0:2,secondCount=zeroSecond?0:2', 'zeroFirst?0:8,secondCount=zeroSecond?0:8')
s=s.replace('void* firstList[]{reinterpret_cast<void*>(&first),reinterpret_cast<void*>(&first)},*secondList[]{reinterpret_cast<void*>(&second),reinterpret_cast<void*>(&second)};', 'Callback firstList[8],secondList[8];for(unsigned i=0;i<8;++i){firstList[i]=first;secondList[i]=second;}fixtureOwner=owner.data();oldLists[0]=firstList;oldLists[1]=secondList;')
s=s.replace('using Callback=void(*)(void*,void*,void*,bool);NativeManagerCallbackAllocatorCandidate allocator;NativeManagerCallbackRegistryCandidateT<NativeManagerCallbackAllocatorCandidate,Callback> a{allocator},b{allocator};', 'MovingAllocator allocatorA,allocatorB;Registry a{allocatorA},b{allocatorB};activeLists[0]=&a;activeLists[1]=&b;')
s=s.replace('std::array<uint32_t,7> record{gates,calls[0],calls[1],firstFlags,secondFlags,firstCount,secondCount};', 'if(appendedCalls)return 8;std::array<uint32_t,8> record{gates,calls[0],calls[1],firstFlags,secondFlags,firstCount,secondCount,appendedCalls};')
s=s.replace('std::array<uint32_t,7> expected{};', 'std::array<uint32_t,8> expected{};')
p.write_text(s)
