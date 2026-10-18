#include "graph_bookmark_replay_composition_fixture.h"
#include "graph_bookmark_purge_candidate.h"
#include "graph_bookmark_storage_purge_candidate.h"
struct PurgeReplay:Replay{
 unsigned purges=0,frees=0;bool flag2c(){return true;}
 struct StoragePurgeAdapter{
  GraphBookmarkStorageFixture& storage;unsigned& frees;
  void setCount0c(int n){storage.count=n;}int growSize8(){return 0;}
  auto* base0(){return storage.data.data();}
  void freeAllocatorSlot5(std::uint32_t* p){assert(p==storage.data.data());++frees;std::vector<std::uint32_t>().swap(storage.data);}
  void setBase0(std::uint32_t* p){assert(p==nullptr&&storage.data.empty());}
  void setCapacity4(int n){storage.cap=n;}
  void setCachedBase10(std::uint32_t* p){storage.cached=p;}
 };
 struct PurgeAdapter:Replay::ClearAdapter{
  unsigned& frees;
  void purgeStorage(GraphBookmarkStorageFixture* p){events.push_back(4);StoragePurgeAdapter a{*p,frees};graphBookmarkStoragePurgeCandidate(a);}
 };
 void finish10377420(int*,GraphBookmarkStorageFixture* s){
  ++purges;PurgeAdapter a{{*this,s,s->count,{}},frees};graphBookmarkPurgeCandidate(a);
  assert((a.events==std::vector<int>{1,2,3,4,5,6,7}));
 }
};
int main(){
 PurgeReplay a;a.old={1,0x80000000u,0xffffffffu};a.next={5,6,7,8};
 for(int i=0;i<100;++i){
  graphBookmarkUndoReplayCandidate(false,a);a.verify(a.old);assert(a.storage.cap==8);
  graphBookmarkUndoReplayCandidate(true,a);a.verify({});
  assert(!a.storage.cap&&a.storage.data.empty()&&!a.storage.cached);
 }
 assert(a.purges==100&&a.frees==100&&a.clearNotifications==300&&a.insertNotifications==700);
 a.next.clear();graphBookmarkUndoReplayCandidate(true,a);assert(a.purges==101&&a.frees==100&&!a.storage.cap);
 graphBookmarkUndoReplayCandidate(false,a);a.verify(a.old);
 std::cout<<"x64 purge-enabled replay: 100 restore/purge cycles, empty redo and undo reallocation passed\n";
}
