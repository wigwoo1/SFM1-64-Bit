from pathlib import Path
p=Path('work/test_restore_blend_nested_original.cpp');assert not p.exists()
s=Path('work/test_restore_blend_original.cpp').read_text().replace('#include "restore_blend_native_init.h"','#include "restore_blend_native_init.h"\n#include "restore_commit_native_init.h"\n#include "restore_commit_setters_native_init.h"')
s=s.replace('event(201,static_cast<unsigned>(kind),offset,n);','event(201,static_cast<unsigned>(kind),offset,n);receiver[0x134]=receiver[0x135]=receiver[0x13c]=receiver[0x13d]=receiver[0x13e]=receiver[0x13f]=1;const uint32_t value=123;std::memcpy(receiver.data()+0x138,&value,4);')
s=s.replace('static void __fastcall commit(void* p,void*){if(p!=receiver.data())std::abort();event(202,0,0);}', '')
at=s.index(' for(auto& fix:restoreBlendFixups)')
loader=''' auto loadBody=[&](const char* path,unsigned size,unsigned offset,const auto& calls,void* depth,void* compare){std::vector<unsigned char> saved(size),actual(size);std::ifstream file(path,std::ios::binary);file.read(reinterpret_cast<char*>(saved.data()),size);if(!file||file.peek()!=std::char_traits<char>::eof())std::abort();dll.clear();dll.seekg(offset);dll.read(reinterpret_cast<char*>(actual.data()),size);if(!dll||actual!=saved)std::abort();auto* body=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!body)std::abort();std::memcpy(body,saved.data(),size);for(auto& call:calls){void* target=nullptr;switch(call[1]){case 0x1000df50:target=reinterpret_cast<void*>(&getter);break;case 0x10002da0:target=reinterpret_cast<void*>(&refill);break;case 0x1002e410:target=depth;break;case 0x1002e490:target=compare;break;}if(!target)std::abort();const uint32_t delta=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(body+call[0]+5));std::memcpy(body+call[0]+1,&delta,4);}DWORD old;if(!VirtualProtect(body,4096,PAGE_EXECUTE_READ,&old))std::abort();FlushInstructionCache(GetCurrentProcess(),body,4096);return body;};
 auto* depth=loadBody("outputs/restore-commit-setter-depth.bin",restoreCommitDepthSize,restoreCommitDepthOffset,restoreCommitDepthCalls,nullptr,nullptr);auto* compare=loadBody("outputs/restore-commit-setter-compare.bin",restoreCommitCompareSize,restoreCommitCompareOffset,restoreCommitCompareCalls,nullptr,nullptr);auto* commitBody=loadBody("outputs/restore-commit-verified-bytes.bin",restoreCommitByteCount,restoreCommitFileOffset,restoreCommitCalls,depth,compare);
'''
s=s[:at]+loader+s[at:]
s=s.replace('#include <cstdlib>','#include <cstdlib>\n#include <vector>')
s=s.replace('reinterpret_cast<void*>(&commit)', 'commitBody')
s=s.replace('13+stages*', '(static_cast<unsigned char>(suppression)?13u:17u)+stages*')
s=s.replace('native-restore-blend.bin','native-restore-blend-nested.bin').replace('restore_blend_original_cases','restore_blend_nested_original_cases')
s=s.replace('VirtualFree(code,0,MEM_RELEASE);','VirtualFree(code,0,MEM_RELEASE);VirtualFree(depth,0,MEM_RELEASE);VirtualFree(compare,0,MEM_RELEASE);VirtualFree(commitBody,0,MEM_RELEASE);')
p.write_text(s)
s=Path('work/test_restore_blend_compare.cpp').read_text().replace('studio_restore_blend_candidate.h','studio_restore_blend_commit_candidate.h')
s=s.replace('event(201,static_cast<unsigned>(kind),offset,n);', 'event(201,static_cast<unsigned>(kind),offset,n);liveState->commit.dirty134=liveState->commit.dirty135=liveState->commit.dirty13c=liveState->commit.enabled13d=liveState->commit.dirty13e=liveState->commit.enabled13f=1;liveState->commit.value138=123;')
s=s.replace('void commit(){event(202,0,0);}', 'void commit(){studioRestoreBlendCommitCandidate(*liveState,*this);}')
s=s.replace('state.commit.cached140=0x55555555;', 'state.commit.value138=state.commit.cached140=0x55555555;')
s=s.replace('const unsigned offsets[]', 'store(0x138,state.commit.value138);store(0x140,state.commit.cached140);receiver[0x13d]=state.commit.enabled13d;receiver[0x13f]=state.commit.enabled13f;const unsigned offsets[]')
s=s.replace('13+stages*', '(static_cast<uint8_t>(suppression)?13u:17u)+stages*')
s=s.replace('native-restore-blend.bin','native-restore-blend-nested.bin').replace('restore_blend_x64_comparison_cases','restore_blend_nested_x64_comparison_cases')
Path('work/test_restore_blend_nested_compare.cpp').write_text(s)
for base in ('original','compare'):
 b=Path('work/build_restore_blend_'+base+'.cmd').read_text().replace('test_restore_blend_'+base,'test_restore_blend_nested_'+base).replace('sfm_blend_original','sfm_blend_nested_original')
 Path('work/build_restore_blend_nested_'+base+'.cmd').write_text(b)
