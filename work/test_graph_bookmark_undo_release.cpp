#include "graph_bookmark_undo_release_candidate.h"
#include <cassert>
#include <iostream>
struct Adapter{
 GraphBookmarkUndoCandidate* object;unsigned bufferFrees=0,objectFrees=0;bool mutate=false;
 void freeBuffer1025eb70(std::uint32_t* p){
  assert(object->identity.vtableTag==0x10b38f80);++bufferFrees;delete[] p;
  if(mutate&&bufferFrees==1){delete[] object->next24;object->next24=new std::uint32_t[7];}
 }
 void freeObject1025eb70(GraphBookmarkUndoCandidate* p){assert(p==object&&p->identity.vtableTag==0x10a8a9fc);++objectFrees;delete p;}
};
int main(){
 for(unsigned i=0;i<1000;++i){auto* o=new GraphBookmarkUndoCandidate;o->old1c=new std::uint32_t[3];o->next24=new std::uint32_t[5];Adapter a{o};a.mutate=i%2!=0;
  assert(graphBookmarkUndoReleaseCandidate(o,0x101,a)==o);assert(a.bufferFrees==2&&a.objectFrees==1);
 }
 GraphBookmarkUndoCandidate retained;Adapter b{&retained};graphBookmarkUndoReleaseCandidate(&retained,0x100,b);
 assert(b.bufferFrees==2&&!b.objectFrees&&retained.identity.vtableTag==0x10a8a9fc);
 std::cout<<"x64 bookmark undo release: 1000 owned records, live second buffer, null buffers and low-byte deletion flag passed\n";
}
