from pathlib import Path
assert not Path('work/test_default_state_composition_native.cpp').exists(), 'One-time migration already applied'
s=Path('work/test_default_render_state.cpp').read_text()
s=s.replace('#include "studio_default_render_state_candidate.h"','#include "studio_default_render_state_owner_candidate.h"\n#include "render_state_restore_native_init.h"')
s=s.replace('static std::array<unsigned char,0x600>', 'static unsigned suppression;static bool queuedMode;static HRESULT __stdcall directState(void*,unsigned key,unsigned value);\nstatic std::array<unsigned char,0x600>')
s=s.replace('#ifdef _M_IX86\nstatic void __fastcall stateCall(void*,void*,unsigned key,unsigned value){submit(key,value);}', 'static HRESULT __stdcall directState(void*,unsigned key,unsigned value){submit(key,value);return E_FAIL;}\n#ifdef _M_IX86\nstatic unsigned char __fastcall gate(void*,void*){return static_cast<unsigned char>(suppression);}')
s=s.replace('struct Services{void submit(unsigned key,unsigned value){::submit(key,value);}', 'struct Services{unsigned suppressed(){return suppression;}bool queueEnabled(){return queuedMode;}void directState(unsigned key,unsigned value){::directState(nullptr,key,value);}void flush(){std::abort();}void publish(uint32_t*){std::abort();}uint32_t* acquire(unsigned){std::abort();}')
needle='auto* code=static_cast<unsigned char*>'
pos=s.index(needle)
s=s[:pos]+'''std::array<unsigned char,sizeof(nativeRenderStateRestoreBytes)> setterBytes{};dll.seekg(nativeRenderStateRestoreOffset);dll.read(reinterpret_cast<char*>(setterBytes.data()),setterBytes.size());if(!dll||std::memcmp(setterBytes.data(),nativeRenderStateRestoreBytes,setterBytes.size()))return 7;auto* setter=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!setter)return 8;std::memcpy(setter,setterBytes.data(),setterBytes.size());setter[54]=0xcc;DWORD setterOld;if(!VirtualProtect(setter,4096,PAGE_EXECUTE_READ,&setterOld))return 9;FlushInstructionCache(GetCurrentProcess(),setter,4096);'''+s[pos:]
s=s.replace('reinterpret_cast<uintptr_t>(&stateCall)', 'reinterpret_cast<uintptr_t>(setter)')
s=s.replace('native-default-render-state.bin','native-default-state-composition.bin')
s=s.replace('unsigned cases=0;for(unsigned flags', 'unsigned cases=0;for(unsigned suppress:{0u,1u,256u})for(bool queued:{false,true})for(unsigned flags')
s=s.replace('trace.clear();', 'trace.clear();suppression=suppress;queuedMode=queued;std::array<uint32_t,256> cache{};cache.fill(99);std::array<uint32_t,512> block{};block.fill(99);unsigned cursor=0,remaining=512;')
s=s.replace('unsigned owner=0;native(&owner);', '''void* ownerTable[37]{};ownerTable[36]=reinterpret_cast<void*>(&gate);void* deviceTable[58]{};deviceTable[57]=reinterpret_cast<void*>(&directState);void* device=deviceTable;std::array<uint32_t,4096> owner{};owner[0]=reinterpret_cast<uintptr_t>(ownerTable);owner[0x648/4]=reinterpret_cast<uintptr_t>(&device);owner[0x658/4]=queued?1:0;owner[0x660/4]=reinterpret_cast<uintptr_t>(block.data());owner[0x664/4]=512;std::memcpy(owner.data()+0x1c98/4,cache.data(),sizeof(cache));native(owner.data());std::memcpy(cache.data(),owner.data()+0x1c98/4,sizeof(cache));cursor=(owner[0x660/4]-reinterpret_cast<uintptr_t>(block.data()))/4;remaining=owner[0x664/4];''')
s=s.replace('Services services;studioDefaultRenderStateCandidate(services);', 'StudioDeviceQueueStateCandidate queue{block.data(),block.data(),512};StudioRenderStateOwnerCandidate owner{cache.data(),&queue};Services services;studioDefaultRenderStateOwnerCandidate(owner,services);cursor=static_cast<unsigned>(queue.cursor-block.data());remaining=queue.remaining;')
s=s.replace('std::array<uint32_t,184> record', 'std::array<uint32_t,954> record')
s=s.replace('record[2+i]=trace[i];', 'record[2+i]=trace[i];record[184]=cursor;record[185]=remaining;std::memcpy(record.data()+186,cache.data(),sizeof(cache));std::memcpy(record.data()+442,block.data(),sizeof(block));')
s=s.replace('std::array<uint32_t,184> expected', 'std::array<uint32_t,954> expected')
s=s.replace('VirtualFree(code,0,MEM_RELEASE);', 'VirtualFree(code,0,MEM_RELEASE);VirtualFree(setter,0,MEM_RELEASE);')
s=s.replace('default_render_state_native_cases','default_state_composition_native_cases')
Path('work/test_default_state_composition_native.cpp').write_text(s)
b=Path('work/build_default_render_state.cmd').read_text().replace('test_default_render_state','test_default_state_composition_native')
Path('work/build_default_state_composition_native.cmd').write_text(b)

