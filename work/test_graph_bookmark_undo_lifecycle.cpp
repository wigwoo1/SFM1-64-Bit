#include "graph_bookmark_undo_lifecycle_fixture.h"
int main(){
 LifecycleReplay replay;const std::vector<std::uint32_t> original{1,0x80000000u,3};
 const std::vector<std::uint32_t> updated{4,5,0xffffffffu,7};auto input=updated;
 replay.storage.data=original;replay.storage.cap=3;replay.storage.count=3;
 replay.record=new GraphBookmarkUndoCandidate;ConstructorAdapter constructor{replay};
 graphBookmarkUndoConstructorCandidate(*replay.record,&replay.attribute,input.data(),int(input.size()),0,constructor);
 assert(constructor.allocations==2&&replay.record->identity.captured8==11&&replay.record->identity.capturedc==12);
 input.assign(4,99);replay.storage.data.assign(3,0);
 for(int i=0;i<100;++i){graphBookmarkUndoReplayCandidate(false,replay);replay.verify(original);graphBookmarkUndoReplayCandidate(true,replay);replay.verify(updated);}
 assert(replay.clearNotifications==200&&replay.insertNotifications==700);
 ReleaseAdapter release;graphBookmarkUndoReleaseCandidate(replay.record,1,release);replay.record=nullptr;
 assert(release.buffers==2&&release.objects==1);
 std::cout<<"x64 composed bookmark undo lifecycle: captured identity/snapshots, 100 replays, notifications and complete record release passed\n";
}
