#pragma once
#include "graph_bookmark_undo_constructor_candidate.h"
template<class Adapter>GraphBookmarkUndoCandidate* graphBookmarkUndoReleaseCandidate(GraphBookmarkUndoCandidate* object,unsigned flags,Adapter& a){
 auto* old=object->old1c;object->identity.vtableTag=0x10b38f80;
 a.freeBuffer1025eb70(old);a.freeBuffer1025eb70(object->next24);
 object->identity.vtableTag=0x10a8a9fc;
 if(static_cast<unsigned char>(flags)&1)a.freeObject1025eb70(object);
 return object;
}
