from pathlib import Path
p=Path('work/prepare_restore_snapshot_native.py');s=p.read_text().replace('0x100211c3','0x100211d4').replace('Span ends before vtable slot183 load/call.', 'Span includes controlled vtable slot183 call with argument1 after owner30d8 byte reset; ends before original epilogue.')
p.write_text(s)
p=Path('work/test_restore_snapshot_native.cpp');s=p.read_text();assert 'finishCalls' not in s
s=s.replace('#include "native_restore_snapshot_candidate.h"','#include "native_restore_snapshot_finish_candidate.h"')
s=s.replace('struct Services{Owner& owner;', '''static Owner* activeOwner;static Owner* expectedAtFinish;static unsigned finishCalls;static bool mutateFinish;
static void finish(bool value){if(!value||*activeOwner!=*expectedAtFinish)std::abort();++finishCalls;if(mutateFinish)reinterpret_cast<unsigned char*>(activeOwner->data())[0x30d8]=7;}
#ifdef _M_IX86
static void* expectedReceiver;static void __fastcall complete(void* receiver,void*,bool value){if(receiver!=expectedReceiver)std::abort();finish(value);}
#endif
struct Services{Owner& owner;
 void clearRestoreForce(){reinterpret_cast<unsigned char*>(owner.data())[0x30d8]=0;}
 void completeRestore(bool value){finish(value);}''')
s=s.replace('for(unsigned seed=0;seed<256;++seed)', 'for(unsigned seed=0;seed<512;++seed)')
s=s.replace('const Owner before=owner;', 'finishCalls=0;mutateFinish=(seed&1)!=0;activeOwner=&owner;const Owner before=owner;')
s=s.replace('expected[slots[i]/4]=before[slots[i]/4];', 'expected[slots[i]/4]=before[slots[i]/4];reinterpret_cast<unsigned char*>(expected.data())[0x30d8]=0;expectedAtFinish=&expected;')
s=s.replace('uint32_t table[1]{};', 'void* table[184]{};table[183]=reinterpret_cast<void*>(&complete);')
s=s.replace('std::array<uint32_t,32> locals{};', 'expectedReceiver=&receiver;std::array<uint32_t,32> locals{};')
s=s.replace('nativeRestoreSnapshotCandidate(services);', 'nativeRestoreSnapshotFinishCandidate(services);')
s=s.replace('if(owner!=expected)return 5;', 'if(finishCalls!=1)return 9;if(mutateFinish)reinterpret_cast<unsigned char*>(expected.data())[0x30d8]=7;if(owner!=expected)return 5;')
s=s.replace('cases=256','cases=512')
p.write_text(s)
