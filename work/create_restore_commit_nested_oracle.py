from pathlib import Path
p=Path('work/test_restore_commit_nested_native.cpp');assert not p.exists()
s=Path('work/test_restore_commit_native.cpp').read_text()
s=s.replace('#include "studio_restore_commit_candidate.h"', '#include "studio_restore_commit_candidate.h"\n#include "studio_restore_cached_state_candidate.h"\n#include "restore_commit_setters_native_init.h"')
s=s.replace('static void __fastcall apply134(void* p,void*,unsigned v){if(p!=receiver.data()||v!=3)std::abort();event(1,v);mutation(1);}', '')
s=s.replace('static void __fastcall apply135(void* p,void*,unsigned v){if(p!=receiver.data()||v!=1)std::abort();event(2,v);mutation(2);}', '')
s=s.replace('void apply134(unsigned v){event(1,v);mutation(1);}void apply135(unsigned v){event(2,v);mutation(2);}', 'void apply134(unsigned v){auto cache=load(0x120);studioRestoreCachedStateCandidate(cache,23,v,*this);store(0x120,cache);}void apply135(unsigned v){auto cache=load(0x11c);studioRestoreCachedStateCandidate(cache,7,v,*this);store(0x11c,cache);}')
needle='auto* code=static_cast<unsigned char*>'
at=s.index(needle)
loader='''auto loadSetter=[&](const char* path,unsigned size,unsigned offset,const auto& calls){std::array<unsigned char,120> saved{},installed{};if(size!=120)std::abort();std::ifstream file(path,std::ios::binary);file.read(reinterpret_cast<char*>(saved.data()),size);if(!file||file.peek()!=std::char_traits<char>::eof())std::abort();dll.clear();dll.seekg(offset);dll.read(reinterpret_cast<char*>(installed.data()),size);if(!dll||installed!=saved)std::abort();auto* body=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!body)std::abort();std::memcpy(body,saved.data(),size);for(auto& call:calls){void* target=call[1]==0x1000df50?reinterpret_cast<void*>(&getter):call[1]==0x10002da0?reinterpret_cast<void*>(&refill):nullptr;if(!target)std::abort();const uint32_t delta=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(body+call[0]+5));std::memcpy(body+call[0]+1,&delta,4);}DWORD old;if(!VirtualProtect(body,4096,PAGE_EXECUTE_READ,&old))std::abort();FlushInstructionCache(GetCurrentProcess(),body,4096);return body;};
 auto* depth=loadSetter("outputs/restore-commit-setter-depth.bin",restoreCommitDepthSize,restoreCommitDepthOffset,restoreCommitDepthCalls);auto* compare=loadSetter("outputs/restore-commit-setter-compare.bin",restoreCommitCompareSize,restoreCommitCompareOffset,restoreCommitCompareCalls);
 '''
s=s[:at]+loader+s[at:]
s=s.replace('reinterpret_cast<void*>(&apply134)', 'compare').replace('reinterpret_cast<void*>(&apply135)', 'depth')
s=s.replace('native-restore-commit.bin','native-restore-commit-nested.bin')
s=s.replace('for(bool change:{false,true})', 'for(bool equal:{false,true})for(bool change:{false,true})')
s=s.replace('store(0x138,0x87654321);', 'store(0x11c,equal?1u:99u);store(0x120,equal?3u:99u);store(0x138,0x87654321);')
begin=s.index(' if(change&&(dirty&1))');end=s.index('\n std::array<uint32_t,227> record',begin)
s=s[:begin]+''' if(load(0x11c)!=((dirty&2)?1u:(equal?1u:99u))||load(0x120)!=((dirty&1)?3u:(equal?3u:99u)))return 9;'''+s[end:]
s=s.replace('VirtualFree(code,0,MEM_RELEASE);','VirtualFree(code,0,MEM_RELEASE);VirtualFree(depth,0,MEM_RELEASE);VirtualFree(compare,0,MEM_RELEASE);')
s=s.replace('restore_commit_original_cases','restore_commit_nested_original_cases')
p.write_text(s)
b=Path('work/build_restore_commit_native.cmd').read_text().replace('test_restore_commit_native','test_restore_commit_nested_native').replace('sfm_commit_oracle','sfm_commit_nested_oracle')
Path('work/build_restore_commit_nested_native.cmd').write_text(b)
