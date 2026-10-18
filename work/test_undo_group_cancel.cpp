#include "undo_group_cancel_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Adapter {
    unsigned char flags=8;std::uint32_t counter=2,depth=1;bool endResult=false;std::vector<int> events;
    auto flags74(){return flags;}auto submissionCounter(){return counter;}
    void logGroupCancel(){events.push_back(1);counter=0;}
    bool endGroup(){events.push_back(2);--depth;return endResult;}
    auto depth54(){return depth;}void undoGroup(){events.push_back(3);}void clearRedoRecords(){events.push_back(4);}
};
int main(){Adapter a;assert(cancelUndoGroupCandidate(a));assert((a.events==std::vector<int>{1,2,3,4}));
    Adapter b;b.depth=2;assert(!cancelUndoGroupCandidate(b));assert((b.events==std::vector<int>{1,2}));
    Adapter c;c.counter=0;c.endResult=true;assert(!cancelUndoGroupCandidate(c));assert(c.events.size()==2);
    Adapter d;d.counter=0xffffffff;assert(!cancelUndoGroupCandidate(d));assert(d.events.size()==2);
    Adapter e;e.flags=0;assert(!cancelUndoGroupCandidate(e));assert(e.events.empty());
    std::cout<<"x64 undo group cancel: pre-log counter snapshot/live depth/ignored end return/signed counter/order passed\n";
}
