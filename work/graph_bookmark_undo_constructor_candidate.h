#pragma once
#include "graph_bookmark_undo_identity_candidate.h"
#include "graph_bookmark_undo_snapshot_candidate.h"
struct GraphBookmarkUndoCandidate{
 GraphAttributeUndoIdentityCandidate identity;
 std::uint32_t* old1c=nullptr;int oldCount20=0;
 std::uint32_t* next24=nullptr;int nextCount28=0;unsigned char flag2c=0;
};
template<class Adapter>struct GraphBookmarkSnapshotFieldsCandidate{
 GraphBookmarkUndoCandidate& object;Adapter& adapter;bool next;
 auto* allocate1025ea80(std::uint32_t bytes){return adapter.allocate1025ea80(bytes);}
 void setBuffer(std::uint32_t* p){(next?object.next24:object.old1c)=p;}
 auto* liveBuffer(){return next?object.next24:object.old1c;}
 void setCount(int n){(next?object.nextCount28:object.oldCount20)=n;}
};
template<class Attribute,class Adapter>void graphBookmarkUndoConstructorCandidate(GraphBookmarkUndoCandidate& o,Attribute* attribute,const std::uint32_t* source,int count,unsigned flag,Adapter& a){
 graphAttributeUndoIdentityCandidate(o.identity,attribute,a.copyAllLabel(),a);
 o.identity.vtableTag=0x10b38f80;o.flag2c=static_cast<unsigned char>(flag);
 auto* vector=attribute&&(a.type10(attribute)&31)==21?a.data4(attribute):nullptr;
 GraphBookmarkSnapshotFieldsCandidate<Adapter> next{o,a,true};graphBookmarkUndoSnapshotCandidate(source,count,next);
 const auto oldCount=a.count0c(vector);auto* oldBase=a.base0(vector);
 GraphBookmarkSnapshotFieldsCandidate<Adapter> old{o,a,false};graphBookmarkUndoSnapshotCandidate(oldBase,oldCount,old);
}
