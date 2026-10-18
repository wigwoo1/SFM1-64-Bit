#pragma once
#include "graph_bookmark_undo_methods_candidate.h"
#include "graph_bookmark_single_insert_candidate.h"
#include "graph_bookmark_storage_composition_fixture.h"
#include <iostream>
#include "graph_bookmark_clear_candidate.h"
#include "graph_bookmark_attribute_insert_candidate.h"
struct Replay {
 GraphBookmarkStorageFixture storage;int owner=0,attribute=0;unsigned clears=0,insertions=0,clearNotifications=0,insertNotifications=0,dirty=0;
 std::vector<std::uint32_t> old,next;
 const char* savedName14(){return "bookmarks";}bool invalidName(const char*){return false;}
 const char* defaultName10a8a4f0(){return "";}
 unsigned savedHandle18(){return 7;}int* resolveModelSlot18(unsigned h){assert(h==7);return &owner;}
 int* find1035dd50(int* p,const char* name){assert(p==&owner&&std::string(name)=="bookmarks");return &attribute;}
 unsigned type10(int*){return 21;}auto* data4(int*){return &storage;}
 struct ClearAdapter {
  Replay& replay;GraphBookmarkStorageFixture* vector;int prior;std::vector<int> events;
  auto* attribute4(){return &replay.attribute;}auto* vector8(){return vector;}
  unsigned mutationGate103623b0(int*){events.push_back(1);return 1;}
  auto* owner8(int*){return &replay.owner;}unsigned recordModelSlot62(int*){events.push_back(2);return 0;}
  int* allocateNativeUndo(){assert(false);return nullptr;}
  int* construct1036c020(int*,int*,std::nullptr_t,int,int){assert(false);return nullptr;}
  void submitModelSlot79(int*){assert(false);}
  unsigned flags11(int*){return 1;}int count0c(GraphBookmarkStorageFixture* p){return p->count;}
  unsigned ownerReady1035dc00(int*){return 1;}
  void ownerSlot2(int*,int*,int start,int end){assert(start==0&&end==graphAliasDifference(prior,1));events.push_back(3);++replay.clearNotifications;}
  void setCount0c(GraphBookmarkStorageFixture* p,int n){events.push_back(4);p->setCount0c(n);}
  void ownerSlot0(int*,int*){assert(vector->count==0);events.push_back(5);}
  unsigned type10(int*){return 21;}
  void markOwner1035eff0(int*,bool refs){assert(!refs);events.push_back(6);}
  void orFrameworkFlags(unsigned bits){replay.dirty|=bits;events.push_back(7);}
 };
 void clear10377310(int*,GraphBookmarkStorageFixture* s){
  ++clears;ClearAdapter adapter{*this,s,s->count,{}};graphBookmarkClearCandidate(adapter);
  assert((adapter.events==std::vector<int>{1,2,3,4,5,6,7}));
 }
 int snapshotCount(bool redo){return int((redo?next:old).size());}
 auto* snapshotBase(bool redo){return (redo?next:old).data();}
 int count0c(GraphBookmarkStorageFixture* s){return s->count0c();}
 struct InsertAdapter:ClearAdapter {
  int* allocateNativeInsertUndo(){assert(false);return nullptr;}
  void constructInsertIdentity10364bd0(int*,int*){assert(false);}
  void setInsertTable(int*,std::uintptr_t){assert(false);}
  void setInsertIndex1c(int*,int){assert(false);}void setInsertValue20(int*,std::uint32_t){assert(false);}
  int insert1008f550(GraphBookmarkStorageFixture* s,int index,const std::uint32_t* value){events.push_back(3);return graphBookmarkSingleInsertCandidate(index,value,*s);}
  void ownerSlot1(int*,int*,int start,int end){assert(start==prior&&end==prior&&vector->count==prior+1);events.push_back(4);++replay.insertNotifications;}
  void ownerSlot0(int*,int*){assert(vector->count==prior+1);events.push_back(5);}
 };
 void insert103770a0(int*,GraphBookmarkStorageFixture* s,int index,std::uint32_t* value){
  ++insertions;InsertAdapter adapter{{*this,s,index,{}}};
  assert(graphBookmarkAttributeInsertCandidate(index,value,adapter)==index);
  assert((adapter.events==std::vector<int>{1,2,3,4,5,6,7}));
 }
 bool flag2c(){return false;}void finish10377420(int*,GraphBookmarkStorageFixture*){assert(false);}
 void verify(const std::vector<std::uint32_t>& expected){
  assert(storage.count==int(expected.size()));
  assert(std::vector<std::uint32_t>(storage.data.begin(),storage.data.begin()+storage.count)==expected);
 }
};
