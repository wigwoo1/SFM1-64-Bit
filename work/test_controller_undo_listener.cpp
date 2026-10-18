#include "controller_undo_listener_candidate.h"
#include <vector>
#include <cassert>
#include <iostream>
struct Child{unsigned char value=0;};
struct Adapter {
    Child child;bool present=true;unsigned char marker=0;std::uint64_t stamp=0;std::vector<int> events;std::uint32_t forwarded=0;
    Child* globalChild344(){return present?&child:nullptr;}
    auto childByte15(Child* c){return c->value;}void setChildByte15(Child* c,unsigned char v){c->value=v;events.push_back(1);}
    void invokeGlobalControllerChange(std::uint32_t v){assert(child.value==static_cast<unsigned char>(v));forwarded=v;events.push_back(2);}
    void setListenerByte219(unsigned char v){marker=v;events.push_back(3);}
    std::uint64_t readTimestampCounter(){assert(marker==1);events.push_back(4);return 0x123456789abcdef0ull;}
    void setListenerTimestamp228(std::uint64_t v){stamp=v;events.push_back(5);}
};
int main(){Adapter a;controllerUndoListenerCandidate(a);assert((a.events==std::vector<int>{1,2,3,4,5}));assert(a.stamp==0x123456789abcdef0ull);
    a.events.clear();controllerUndoListenerCandidate(a);assert((a.events==std::vector<int>{1,3,4,5}));
    Adapter b;b.present=false;controllerUndoListenerCandidate(b);assert((b.events==std::vector<int>{3,4,5}));
    Adapter c;updateControllerUndoChildCandidate(&c.child,0x123401,c);assert(c.child.value==1 && c.forwarded==0x123401);
    std::cout<<"x64 controller undo listener: changed/unchanged/absent child/flag before timestamp/full input forwarding passed\n";
}
