#pragma once
#include <vector>
#include <utility>
#include <type_traits>
#include <limits>
#include <stdexcept>
#include "undo_recording_suppression.h"
// Recovered stack ordering/boundary rules of 10390D50 and 10390E50.
// Transaction labels, notifications, native ownership and capture are separate.
template<class Record>struct RuntimeUndoJournal{
 struct Entry{Record record;bool boundary=false;};
 uint8_t flags=8;bool pendingBoundary=false;std::vector<Entry>undo,redo;
 void beginBoundary(){pendingBoundary=true;}
 template<class Visit>void forEachRecord(Visit visit){for(auto&entry:undo)visit(entry.record);for(auto&entry:redo)visit(entry.record);}
 void append(Record record){
  static_assert(std::is_nothrow_move_constructible<Record>::value,"undo records must move without throwing");
  // A failed history allocation must preserve the existing redo branch and
  // pending boundary. Only discard redo after the new entry is committed.
  undo.push_back({std::move(record),pendingBoundary});redo.clear();pendingBoundary=false;
 }
 // Commit a fully prepared group without copying prior history. Reserve before
 // mutating stacks; nothrow record moves make the remaining commit atomic.
 void appendGroup(std::vector<Record>records){
  static_assert(std::is_nothrow_move_constructible<Record>::value,"group records must move without throwing");
  if(records.empty())return;if(records.size()>undo.max_size()-undo.size())throw std::runtime_error("undo group capacity overflow");
  size_t required=undo.size()+records.size();if(required>undo.capacity()){size_t capacity=undo.capacity();size_t grown=capacity>undo.max_size()/2?undo.max_size():capacity*2;undo.reserve(required>grown?required:grown);}
  redo.clear();bool first=true;for(auto&record:records){undo.push_back({std::move(record),first});first=false;}pendingBoundary=false;
 }
 // Replay callbacks must not mutate journal stacks. Preserve a failing entry;
 // callback side effects are not rolled back. Reserve before replay so a
 // history allocation failure cannot follow a successful payload mutation.
 void reserveTransfer(std::vector<Entry>&destination){static_assert(std::is_nothrow_move_constructible<Record>::value,"replay records must move without throwing");if(destination.size()==destination.max_size())throw std::runtime_error("undo transfer capacity overflow");if(destination.size()==destination.capacity()){auto capacity=destination.capacity();auto grown=capacity>destination.max_size()/2?destination.max_size():capacity*2;destination.reserve(grown>destination.size()?grown:destination.size()+1);}}
 template<class Replay>size_t reverse(Replay replay){RuntimeUndoRecordingSuppression guard(flags);size_t count=0;
  while(!undo.empty()){reserveTransfer(redo);auto entry=std::move(undo.back());undo.pop_back();try{replay(entry.record,true);}catch(...){undo.push_back(std::move(entry));throw;}bool stop=entry.boundary;redo.push_back(std::move(entry));++count;if(stop)break;}return count;}
 template<class Replay>size_t forward(Replay replay){RuntimeUndoRecordingSuppression guard(flags);size_t count=0;
  while(!redo.empty()){reserveTransfer(undo);auto entry=std::move(redo.back());redo.pop_back();try{replay(entry.record,false);}catch(...){redo.push_back(std::move(entry));throw;}undo.push_back(std::move(entry));++count;if(redo.empty()||redo.back().boundary)break;}return count;}
};
