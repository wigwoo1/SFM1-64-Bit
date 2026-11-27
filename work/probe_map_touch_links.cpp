#include "source_map_touch_links_candidate.h"

#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string_view>

constexpr unsigned count = 6;
using Event = SourceMapTouchEvent;
std::ostream* output = nullptr;
bool queueMode = false;
void emit(Event event, uint32_t owner, uint32_t peer) {
    *output << ' ' << static_cast<uint32_t>(event) << ':' << owner << ':' << peer;
}

#ifdef NATIVE_MAP_TOUCH_LINKS
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"

template<class T> T& at(uintptr_t address) { return *reinterpret_cast<T*>(address); }
struct Interface { const uintptr_t* table; void* object; };
struct Node { uint32_t peer, stamp; Node *next, *previous; uint32_t flags; };
static_assert(sizeof(Node) == 20);
using Entity = std::array<uint32_t, 0x370/4>;
std::array<Entity,count> entities{};
std::array<Node,count> heads{};
std::array<Node,64> pool{};
std::array<bool,64> used{};
std::array<std::array<uintptr_t,4>,count> slots{}, edicts{};
std::array<Interface,count> networkables{};
std::array<uintptr_t,0x1a4/4> entityTable{};
std::array<uint32_t,16> debugControl{};
std::array<void*,64> nativeQueue{};
unsigned nativeRequeue=count;
unsigned indexOf(const void* object) {
    for (unsigned i=0;i<count;++i) if (entities[i].data()==object) return i;
    throw std::runtime_error("unknown native entity");
}
const uintptr_t* __fastcall handle(void* self, void*) { return &slots[indexOf(self)][2]; }
void* __fastcall networkEntity(Interface* self, void*) { return self->object; }
Node* __fastcall getHead(void*,void*,void* who) { return &heads[indexOf(who)]; }
Node* __fastcall createHead(void*,void*,void* who) { return &heads[indexOf(who)]; }
void __fastcall freeHead(void*,void*,void* who) {
    auto& h=heads[indexOf(who)];
    if(h.next!=&h || h.previous!=&h) throw std::runtime_error("nonempty touch list reclaimed");
    h={};
}
template<Event E> void __fastcall touch(void* owner,void*,void* peer) {
    emit(E,slots[indexOf(owner)][2],slots[indexOf(peer)][2]);
    if constexpr(E==Event::end) {
        if(queueMode && nativeRequeue<count) {
            auto& entity=entities[std::exchange(nativeRequeue,count)];
            reinterpret_cast<void(__thiscall*)(void*,bool)>(0x100bc4c0)(entity.data(),true);
        }
    }
}
void boundary(unsigned which, CONTEXT& context) {
    using Observer = PrivateX86BoundaryObserverFixture;
    const auto* args = reinterpret_cast<const uintptr_t*>(context.Esp);
    if (which==0) {
        if(args[1]!=sizeof(Node)) throw std::runtime_error("unexpected touch allocation");
        const auto slot=std::find(used.begin(),used.end(),false);
        if(slot==used.end()) throw std::runtime_error("touch pool exhausted");
        *slot=true;
        Observer::returnCdecl(context,reinterpret_cast<uintptr_t>(&pool[slot-used.begin()]));
        context.Esp+=4;
    } else if (which==1) {
        unsigned i=0;
        while (i<pool.size() && reinterpret_cast<uintptr_t>(&pool[i])!=args[1]) ++i;
        if(i==pool.size() || !used[i]) throw std::runtime_error("unexpected touch free");
        used[i]=false;
        Observer::returnCdecl(context);
        context.Esp+=4;
    } else if (which==2) {
        const auto id=slots[indexOf(reinterpret_cast<void*>(args[1]))][2];
        emit(Event::schedule,id,id);
        Observer::returnCdecl(context);
    } else {
        void* destination=reinterpret_cast<void*>(args[1]);
        std::memset(destination,static_cast<int>(args[2]),args[3]);
        Observer::returnCdecl(context,args[1]);
    }
}

struct Fixture {
    Fixture() {
        entities={}; heads={}; pool={}; used={};
        static const uintptr_t headTable[]{0,reinterpret_cast<uintptr_t>(getHead),
            reinterpret_cast<uintptr_t>(createHead),reinterpret_cast<uintptr_t>(freeHead)};
        static const uintptr_t networkTable[]{0,0,0,0,0,reinterpret_cast<uintptr_t>(networkEntity)};
        static Interface service{headTable,nullptr};
        at<Interface*>(0x1063cb08)=&service;
        at<uintptr_t>(0x105a01e0)=reinterpret_cast<uintptr_t>(slots.data())-4;
        at<uint32_t*>(0x1063ca8c)=debugControl.data();
        at<uint32_t>(0x1063c9a8)=0;
        at<uint8_t>(0x105c159c)=1;
        at<uint8_t>(0x106065d2)=0;
        if(queueMode) {
            nativeRequeue=count;
            at<void**>(0x105a4240)=nativeQueue.data();
            at<uint32_t>(0x105a4244)=static_cast<uint32_t>(nativeQueue.size());
            at<uint32_t>(0x105a4248)=0;
            at<uint32_t>(0x105a424c)=0;
        }
        entityTable[2]=reinterpret_cast<uintptr_t>(handle);
        entityTable[0x198/4]=reinterpret_cast<uintptr_t>(touch<Event::start>);
        entityTable[0x19c/4]=reinterpret_cast<uintptr_t>(touch<Event::touch>);
        entityTable[0x1a0/4]=reinterpret_cast<uintptr_t>(touch<Event::end>);
        for(unsigned i=0;i<count;++i) {
            auto* object=entities[i].data();
            slots[i]={reinterpret_cast<uintptr_t>(object),1,0x10000u+i,0};
            networkables[i]={networkTable,object};
            edicts[i][3]=reinterpret_cast<uintptr_t>(&networkables[i]);
            entities[i][0]=reinterpret_cast<uintptr_t>(entityTable.data());
            entities[i][0x158/4]=UINT32_MAX;
        }
    }
    void step(char op,unsigned a,unsigned b) {
        auto* object=entities.at(a).data();
        if(op=='b') reinterpret_cast<void(__thiscall*)(void*,bool)>(0x100bc4c0)(object,true);
        if(op=='f') reinterpret_cast<void(__thiscall*)(void*)>(0x1019baf0)(object);
        if(op=='x') reinterpret_cast<void(__thiscall*)(void*,bool)>(0x100bc4c0)(object,false);
        if(op=='j' && queueMode) nativeRequeue=a;
        if(op=='u' && queueMode) {
            std::array<uint32_t,4> physicsSystem{};
            reinterpret_cast<void(__thiscall*)(void*)>(0x10110590)(physicsSystem.data());
        }
        if(op=='c' && slots[a][0] && slots.at(b)[0]) {
            using Mark = void(__thiscall*)(void*,void*,void*);
            const auto entry=at<uintptr_t>(0x10495564+4);
            reinterpret_cast<Mark>(entry)(nullptr,edicts[a].data(),edicts[b].data());
        }
        if(op=='s') {
            const uint32_t flags=((b&2)?8u:0u)|((b&4)?32u:0u);
            object[0x184/4]=flags|((b&1)?2u<<16:0);
            object[0x11c/4]=(object[0x11c/4]&~1u)|((b&8)?1u:0u);
            object[0x120/4]=(b&16)?0x400000u:0;
        }
        if(op=='p') object[0x158/4]=b<count?slots[b][2]:UINT32_MAX;
        if(op=='h') {
            slots[a][0]=b?reinterpret_cast<uintptr_t>(object):0;
            if(b) { ++slots[a][1]; slots[a][2]+=0x10000u; }
        }
        if(op=='q') at<uint8_t>(0x106065d2)=static_cast<uint8_t>(b);
        if(op=='e') object[0x84/4]=b;
    }
    void state() const {
        unsigned links=0;
        for(unsigned i=0;i<count;++i) {
            const auto& o=entities[i];
            *output << ' ' << o[0x84/4] << ':' << bool(o[0x11c/4]&0x1000000);
            if(!(o[0x36c/4]&2)) continue;
            const auto* head=&heads[i];
            for(auto* p=head->next;p!=head;p=p->next) {
                if(!p || ++links>pool.size() || p->next->previous!=p) throw std::runtime_error("invalid native touch chain");
                *output << ',' << p->peer << ':' << p->stamp << ':' << p->flags;
            }
        }
        if(links!=at<uint32_t>(0x1063c9a8) || links!=static_cast<unsigned>(std::count(used.begin(),used.end(),true)))
            throw std::runtime_error("native touch allocation count mismatch");
        if(queueMode) {
            *output << " |";
            for(unsigned i=0;i<at<uint32_t>(0x105a424c);++i)
                *output << ' ' << slots[indexOf(nativeQueue.at(i))][2];
        }
    }
};
#else
struct Fixture {
    std::array<SourceMapTouchEntity,count> entities{};
    std::array<uint32_t,count> ids{65536,65537,65538,65539,65540,65541};
    std::array<bool,count> live{true,true,true,true,true,true};
    bool suppressed=false;
    SourceMapUntouchQueueCandidate pending;
    unsigned requeue=count;
    void step(char op,unsigned a,unsigned b) {
        auto& object=entities.at(a);
        const auto lookup=[&](uint32_t id)->SourceMapTouchEntity* {
            const unsigned i=id&0xffff;
            return i<count && live[i] && ids[i]==id?&entities[i]:nullptr;
        };
        const auto begin=[&](unsigned index) {
            sourceBeginTouchScan(entities[index],ids[index],[&](Event event,uint32_t owner,uint32_t peer) {
                if(queueMode) pending.schedule(owner,entities[index]);
                else emit(event,owner,peer);
            });
        };
        const auto notify=[&](Event event,uint32_t owner,uint32_t peer) {
            emit(event,owner,peer);
            if(event==Event::end && queueMode && requeue<count) begin(std::exchange(requeue,count));
        };
        if(op=='b') begin(a);
        if(op=='f') sourceFinishTouchScan(object,ids[a],lookup,notify);
        if(op=='x') object.pending=false;
        if(op=='j' && queueMode) requeue=a;
        if(op=='u' && queueMode) pending.service(lookup,[&](uint32_t id) {
            sourceFinishTouchScan(*lookup(id),id,lookup,notify);
        });
        if(op=='c' && live[a] && live.at(b)) {
            // An expired parent never participates in admission.
            if(!lookup(object.parent)) object.parent=UINT32_MAX;
            if(!lookup(entities[b].parent)) entities[b].parent=UINT32_MAX;
            sourceReportMapTouch(object,ids[a],entities[b],ids[b],suppressed,emit);
        }
        if(op=='s') {
            object.solid=(b&1)!=0;
            object.trigger=(b&2)!=0;
            object.suppressSolidTouch=(b&4)!=0;
            object.deleting=(b&8)!=0;
            object.markedForRemoval=(b&16)!=0;
        }
        if(op=='p') object.parent=b<count?ids[b]:UINT32_MAX;
        if(op=='h') {
            live[a]=b!=0;
            if(b) ids[a]+=0x10000u;
        }
        if(op=='q') suppressed=b!=0;
        if(op=='e') object.stamp=b;
    }
    void state() const {
        for(const auto& e:entities) {
            *output << ' ' << e.stamp << ':' << e.pending;
            for(const auto& link:e.links) *output << ',' << link.peer << ':' << link.stamp << ':' << link.notifyEnd;
        }
        if(queueMode) {
            *output << " |";
            for(const auto id:pending.queued()) *output << ' ' << id;
        }
    }
};
#endif

int main(int argc,char** argv) {
    try {
#ifdef NATIVE_MAP_TOUCH_LINKS
        queueMode=argc==5 && std::string_view(argv[4])=="--queue";
        if(argc!=4 && !queueMode) throw std::runtime_error("expected input output server paths");
        NativePrivateWorldImageFixture image(argv[3],0x6de6c4);
        image.seal();
        PrivateX86BoundaryObserverFixture observer{
            {0x103893f0,0x10389310,queueMode?0x10002500u:0x10111000u,0x103ec0b0},boundary};
        observer.configure(true);
#else
        queueMode=argc==4 && std::string_view(argv[3])=="--queue";
        if(argc!=3 && !queueMode) throw std::runtime_error("expected input and output paths");
#endif
        std::ifstream input(argv[1]);
        std::ofstream file(argv[2],std::ios::binary);
        if(!input || !file) throw std::runtime_error("fixture I/O");
        output=&file;
        Fixture fixture;
        char op;
        unsigned a,b;
        unsigned rows=0;
        while(input>>op>>a>>b) {
            if(op=='r') fixture=Fixture{};
            else fixture.step(op,a,b);
            file << " |";
            fixture.state();
            file << '\n';
            ++rows;
        }
        if(!rows || !input.eof() || !file) throw std::runtime_error("invalid touch fixture stream");
#ifdef NATIVE_MAP_TOUCH_LINKS
        observer.configure(false);
        image.verify();
#endif
        std::cout << "touch lifecycle rows=" << rows << '\n';
        return 0;
    } catch(const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
