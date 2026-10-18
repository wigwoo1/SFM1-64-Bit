from pathlib import Path
assert not Path('work/test_restore_tail_native.cpp').exists(), 'One-time fixture migration already applied; maintain resulting source directly'
s=Path('work/test_restore_snapshot_native.cpp').read_text()
s=s.replace('restore_snapshot_native_init.h','restore_tail_native_init.h').replace('restoreSnapshot','restoreTail').replace('restore-snapshot-verified','restore-tail-verified').replace('native-restore-snapshot.bin','native-restore-tail.bin')
s=s.replace('#include "native_restore_snapshot_finish_candidate.h"', '#include "native_restore_dispatch_snapshot_finish_candidate.h"\n#include "native_manager_callback_registry_candidate.h"')
s=s.replace('static Owner* activeOwner;', '''static unsigned gates,callbackCalls[2];static bool suppressed[2];
static void* expectedArgs[3];
static void callback(unsigned category,void* command,void* current,void* previous,bool force){if(command!=expectedArgs[0]||current!=expectedArgs[1]||previous!=expectedArgs[2]||!force)std::abort();++callbackCalls[category];static_cast<uint32_t*>(previous)[17+category*2]=0xabc00000u+category;}
static void first(void* c,void* a,void* b,bool f){callback(0,c,a,b,f);}
static void second(void* c,void* a,void* b,bool f){callback(1,c,a,b,f);}
#ifdef _M_IX86
static bool __fastcall gate(void*,void*){return suppressed[gates++];}
#endif
static Owner* activeOwner;''')
s=s.replace('if(!value||*activeOwner', 'if(gates!=2||!value||*activeOwner')
s=s.replace('const Owner before=owner;', '''gates=callbackCalls[0]=callbackCalls[1]=0;suppressed[0]=(seed&2)!=0;suppressed[1]=(seed&4)!=0;const unsigned counts[]{(seed&8)?0u:2u,(seed&16)?0u:2u};
  auto* base=reinterpret_cast<unsigned char*>(owner.data());expectedArgs[0]=base+0x648;expectedArgs[1]=base+0x2010;expectedArgs[2]=base+0x1198;
  owner[0]=owner[0x2e8c/4]=owner[0x2ea0/4]=0;owner[0x2e98/4]=counts[0];owner[0x2eac/4]=counts[1];base[0x2e88]=0x82;base[0x2e89]=0x81;
#ifdef _M_IX86
  void* ownerTable[37]{};ownerTable[36]=reinterpret_cast<void*>(&gate);void* list0[]{reinterpret_cast<void*>(&first),reinterpret_cast<void*>(&first)},*list1[]{reinterpret_cast<void*>(&second),reinterpret_cast<void*>(&second)};owner[0]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ownerTable));owner[0x2e8c/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(list0));owner[0x2ea0/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(list1));
#endif
  Owner before=owner;for(unsigned category=0;category<2;++category)if(counts[category]&&!suppressed[category]){before[(0x1198/4)+17+category*2]=0xabc00000u+category;reinterpret_cast<unsigned char*>(before.data())[0x2e88+category]=0;before[(category?0x2eac:0x2e98)/4]=0;}''')
s=s.replace('Services services{owner};nativeRestoreSnapshotFinishCandidate(services);', '''Services services{owner};using Callback=void(*)(void*,void*,void*,bool);NativeManagerCallbackAllocatorCandidate allocator;NativeManagerCallbackRegistryCandidateT<NativeManagerCallbackAllocatorCandidate,Callback> a{allocator},b{allocator};for(unsigned i=0;i<counts[0];++i)a.append(first);for(unsigned i=0;i<counts[1];++i)b.append(second);
  // Publish native-layout count/flag cleanup before completion observation.
  auto liveGate=[&]{const unsigned category=gates++;if(counts[category]&&!suppressed[category]){base[0x2e88+category]=0;owner[(category?0x2eac:0x2e98)/4]=0;}return suppressed[category];};
  nativeRestoreDispatchSnapshotFinishCandidate(a,base[0x2e88],b,base[0x2e89],liveGate,services,expectedArgs[0],expectedArgs[1],expectedArgs[2],true);''')
s=s.replace('if(owner!=expected)return 5;', '''if(owner!=expected)return 5;for(unsigned category=0;category<2;++category)if(callbackCalls[category]!=(suppressed[category]?0u:counts[category]))return 10;
  owner[0]=owner[0x2e8c/4]=owner[0x2ea0/4]=0;''')
s=s.replace('restore_snapshot_original_cases','restore_tail_original_cases')
Path('work/test_restore_tail_native.cpp').write_text(s)
b=Path('work/build_restore_snapshot_native.cmd').read_text().replace('test_restore_snapshot_native','test_restore_tail_native').replace('sfm_snapshot_oracle','sfm_tail_oracle')
Path('work/build_restore_tail_native.cmd').write_text(b)

