#include "undo_record_submission_candidate.h"
#include <vector>
#include <cassert>
#include <iostream>
struct Record{};
struct Adapter {
    std::vector<Record*> records;std::vector<unsigned> events;
    std::uint32_t counter=0xffffffff;std::uint8_t flags=1;
    auto field70(){return counter;}void setField70(std::uint32_t v){counter=v;events.push_back(1);}
    void clearRedoRecords(){assert(counter==0);events.push_back(2);}
    const char* recordLabel(Record*){events.push_back(3);return "fixture";}
    void logSubmittedRecord(const char*){events.push_back(4);}
    int count(){return static_cast<int>(records.size());}int capacity(){return static_cast<int>(records.capacity());}
    void growRecords(int n){assert(n==1);records.reserve(16);events.push_back(5);}
    void incrementCountAndRefreshBase(){records.push_back(nullptr);events.push_back(6);}
    Record** lastRecordSlot(){return &records.back();}
    auto flags74(){return flags;}
    void setRecordMarker(Record* p,int v){assert(v==1 && records.back()==p);flags|=0x10;events.push_back(7);}
    void setFlags74(std::uint8_t v){flags=v;events.push_back(8);}
};
int main(){Adapter a;Record r;
    submitUndoRecordCandidate(static_cast<Record*>(nullptr),a);assert(a.events.empty() && a.counter==0xffffffff);
    submitUndoRecordCandidate(&r,a);assert(a.counter==0 && a.records[0]==&r && a.flags==0x10);
    assert((a.events==std::vector<unsigned>{1,2,3,4,5,6,7,8}));
    std::cout<<"x64 undo submission: null noop/counter wrap/helper order/native pointer append/live flags passed\n";
}
