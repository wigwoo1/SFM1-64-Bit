#include "undo_group_begin_candidate.h"
#include "undo_group_end_candidate.h"
#include "undo_group_cancel_candidate.h"
#include "undo_record_submission_candidate.h"
#include "undo_operation_candidate.h"
#include "redo_operation_candidate.h"
#include <vector>
#include <cassert>
#include <iostream>
struct Record { int before,after;unsigned char marker=0;bool disposed=false; };
struct Stack {
    std::vector<Record*> undo,redo;unsigned char flags=8;
    std::uint32_t depth=0,counter=0;std::uintptr_t context=0,previous=0,first=0,second=0;
    int value=0,notifications=0;
    auto flags74(){return flags;}void setFlags74(unsigned char v){flags=v;}
    auto depth54(){return depth;}void setDepth54(std::uint32_t v){depth=v;}
    auto currentContext(){return context;}auto previousContext(){return previous;}
    void setCurrentContext(std::uintptr_t v){context=v;}void setPreviousContext(std::uintptr_t v){previous=v;}
    std::uintptr_t internLabel(const char* p){return reinterpret_cast<std::uintptr_t>(p);}
    void setFirstLabel(std::uintptr_t v){first=v;}void setSecondLabel(std::uintptr_t v){second=v;}
    auto field70(){return counter;}auto submissionCounter(){return counter;}
    void setField70(std::uint32_t v){counter=v;}void setSubmissionCounter(std::uint32_t v){counter=v;}
    void logGroupBegin(const char*){}void logGroupEnd(){}void logGroupCancel(){}
    void clearRedoRecords(){for(auto* r:redo){assert(!r->disposed);r->disposed=true;}redo.clear();}
    int count(){return undoCount();}int capacity(){return static_cast<int>(undo.capacity());}
    void growRecords(int n){assert(n==1);undo.reserve(undo.size()+1);}
    void incrementCountAndRefreshBase(){undo.push_back(nullptr);}Record** lastRecordSlot(){return &undo.back();}
    int recordLabel(Record*){return 0;}void logSubmittedRecord(int){}void logRecord(int){}
    auto recordMarker(Record* r){return r->marker;}void setRecordMarker(Record* r,int v){r->marker=static_cast<unsigned char>(v);}
    int undoCount(){return static_cast<int>(undo.size());}int redoCount(){return static_cast<int>(redo.size());}
    Record* undoRecord(int i){return undo.at(i);}Record* liveRedoTail(){return redo.back();}
    Record* popLiveUndoTail(){auto* r=undo.back();undo.pop_back();return r;}
    Record* popLiveRedoTail(){auto* r=redo.back();redo.pop_back();return r;}
    void replayUndo(Record* r){assert(!(flags&8) && !r->disposed);value=r->before;}
    void replayRedo(Record* r){assert(!(flags&8) && !r->disposed);value=r->after;}
    void appendUndo(Record* r){undo.push_back(r);}void appendRedo(Record* r){redo.push_back(r);}
    void beginUndoNotification(){++notifications;}void beginRedoNotification(){++notifications;}
    void logUndoBegin(){}void logUndoEnd(){}void logRedoBegin(){}void logRedoEnd(){}
    int field54(){return static_cast<int>(depth);}void clearFields78And7c(){context=previous=0;}
    void endUndoNotification(int v){assert(v==0);--notifications;}void endRedoNotification(int v){assert(v==0);--notifications;}
    bool endGroup(){return endUndoGroupCandidate(*this);}void undoGroup(){undoOperationCandidate(*this);}
    void edit(Record& r){r.before=value;submitUndoRecordCandidate(&r,*this);value=r.after;}
};
int main(){
    const char label[]="edit";
    Stack s;Record a{0,1},b{0,2},c{0,3};
    beginUndoGroupCandidate(label,label,1,s);s.edit(a);s.edit(b);assert(s.endGroup());
    beginUndoGroupCandidate(label,label,2,s);s.edit(c);assert(s.endGroup());
    undoOperationCandidate(s);assert(s.value==2);undoOperationCandidate(s);assert(s.value==0);
    redoOperationCandidate(s);assert(s.value==2);redoOperationCandidate(s);assert(s.value==3 && !s.notifications);
    Stack merged;Record x{0,1},y{0,2};
    beginUndoGroupCandidate(label,label,0x100000001ull,merged);merged.edit(x);assert(merged.endGroup());
    beginUndoGroupCandidate(label,label,0x100000001ull,merged);merged.edit(y);
    assert(cancelUndoGroupCandidate(merged));assert(merged.value==0 && merged.undo.empty() && merged.redo.empty() && x.disposed && y.disposed);
    std::cout<<"x64 composed undo stack: group submission/two-group value roundtrip/matching-context cancel merges earlier edit passed\n";
}
