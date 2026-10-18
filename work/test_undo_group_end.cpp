#include "undo_group_end_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Record{unsigned char marker;};
struct Adapter {
    unsigned char flags=8;std::uint32_t depth=1,counter=2;std::uintptr_t current=0x100000001ull,previous=current;int clears=0,logs=0;
    Record a{1},b{128},c{0};std::vector<Record*> records{&a,&b,nullptr,&c};
    auto flags74(){return flags;}void logGroupEnd(){++logs;}
    auto depth54(){return depth;}void setDepth54(std::uint32_t d){depth=d;}
    auto submissionCounter(){return counter;}void setSubmissionCounter(std::uint32_t c0){counter=c0;}
    void clearRedoRecords(){++clears;}auto currentContext(){return current;}auto previousContext(){return previous;}
    int undoCount(){return static_cast<int>(records.size());}Record* undoRecord(int i){return records.at(i);}
    auto recordMarker(Record* r){return r->marker;}void setRecordMarker(Record* r,int v){r->marker=static_cast<unsigned char>(v);}
};
int main(){Adapter a;assert(endUndoGroupCandidate(a));assert(!a.depth && !a.counter && a.clears==1 && !a.b.marker && a.a.marker==1);
    Adapter b;b.depth=2;assert(!endUndoGroupCandidate(b) && b.depth==1 && b.counter==2 && !b.clears);
    Adapter c;c.previous=0;assert(endUndoGroupCandidate(c) && c.b.marker==128);
    Adapter d;d.counter=0x80000000u;assert(!endUndoGroupCandidate(d) && !d.counter && !d.clears);
    Adapter e;e.flags=0;assert(!endUndoGroupCandidate(e) && e.depth==1 && !e.logs);
    Adapter f;f.depth=0;assert(!endUndoGroupCandidate(f) && f.depth==0xffffffff && f.counter==2);
    std::cout<<"x64 undo group end: context merge/null scan/nested/disabled/signed counter/raw underflow passed\n";
}
