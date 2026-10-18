#pragma once
#include "graph_bookmark_replay_composition_fixture.h"
#include "graph_bookmark_undo_constructor_candidate.h"
#include "graph_bookmark_undo_release_candidate.h"
struct LifecycleReplay:Replay{
 GraphBookmarkUndoCandidate* record=nullptr;
 std::uintptr_t savedName14(){return record->identity.name14;}
 bool invalidName(std::uintptr_t n){return n==~std::uintptr_t(0);}
 std::uintptr_t defaultName10a8a4f0(){return 0;}
 unsigned savedHandle18(){return record->identity.handle18;}
 int* find1035dd50(int* p,std::uintptr_t name){assert(p==&owner&&name==0x100000044ull);return &attribute;}
 int snapshotCount(bool redo){return redo?record->nextCount28:record->oldCount20;}
 auto* snapshotBase(bool redo){return redo?record->next24:record->old1c;}
 bool flag2c(){return record->flag2c!=0;}
};
struct ConstructorAdapter{
 LifecycleReplay& replay;unsigned allocations=0;
 const char* copyAllLabel(){return "CUndoAttributeArrayCopyAllElement";}
 unsigned captureModelSlot80(const char*){return 11;}unsigned captureModelSlot81(const char*){return 12;}
 auto* owner8(int*){return &replay.owner;}unsigned handle1c(int*){return 7;}
 std::uintptr_t name14(int*){return 0x100000044ull;}
 unsigned type10(int*){return 21;}auto* data4(int*){return &replay.storage;}
 int count0c(GraphBookmarkStorageFixture* p){return p->count;}auto* base0(GraphBookmarkStorageFixture* p){return p->base0();}
 auto* allocate1025ea80(unsigned bytes){++allocations;return new std::uint32_t[bytes/4];}
};
struct ReleaseAdapter{
 unsigned buffers=0,objects=0;
 void freeBuffer1025eb70(std::uint32_t* p){++buffers;delete[] p;}
 void freeObject1025eb70(GraphBookmarkUndoCandidate* p){++objects;delete p;}
};
