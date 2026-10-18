#include "undo_redo_cleanup_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Record { int id; };
struct Context { void* base=nullptr; int capacity=0,growSize=0,count=0; void* cachedBase=nullptr; };
struct Adapter {
    Record first{1},second{2},replacement{3};
    std::vector<Record*> records{&first,&second};
    std::vector<int> events;
    int count=2,growSize=0; unsigned char enabled=1; int allocation=0;
    int redoCount(){return count;}
    Context makeZeroContext(){events.push_back(1);return {};}
    void collectContext(Context& c){events.push_back(2);c.base=&allocation;c.growSize=growSize;c.capacity=4;c.count=1;}
    void beginContext(Context&){events.push_back(3);}
    auto recordingEnabled(){events.push_back(4);return enabled;}
    void setRecordingEnabled(unsigned char v){enabled=v;events.push_back(v?9:5);}
    Record* redoRecord(int i){return records.at(i);}
    int recordLabel(Record* r){assert(!enabled);return r->id;}
    void logClearedRecord(int id){events.push_back(10+id);}
    void invokeRecordSlot5(Record* r){assert(!enabled);events.push_back(20+r->id);if(r==&first){records[1]=&replacement;count=99;}}
    void setRedoCount(int n){count=n;events.push_back(6);}
    void endContext(Context&){assert(count==0 && !enabled);events.push_back(7);}
    void freeContext(void* p){assert(p==&allocation && enabled);events.push_back(8);}
};
int main(){
    Adapter empty;empty.count=0;clearRedoRecordsCandidate(empty);assert(empty.events.empty());
    Adapter a;clearRedoRecordsCandidate(a);
    assert((a.events==std::vector<int>{1,2,3,4,5,11,21,13,23,6,7,9,8}));
    assert(a.count==0 && a.enabled==1);
    Adapter external;external.growSize=-1;clearRedoRecordsCandidate(external);
    assert(external.events.back()==9);
    Adapter negative;negative.count=-1;clearRedoRecordsCandidate(negative);
    assert(negative.count==0 && negative.enabled==1);
    std::cout<<"x64 redo cleanup: empty/snapshot count/live vector/recording scope/context ownership passed\n";
}
