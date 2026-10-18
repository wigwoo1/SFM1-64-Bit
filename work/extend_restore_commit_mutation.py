from pathlib import Path
p=Path('work/test_restore_commit_native.cpp');s=p.read_text();assert 'mutateCallbacks' not in s
s=s.replace('static unsigned eventCount', 'static bool mutateCallbacks;\n#ifndef _M_IX86\nstatic StudioRestoreCommitStateCandidate* liveState;\n#endif\nstatic unsigned eventCount')
at='static HRESULT __stdcall direct('
pos=s.index(at)
helper='''static void mutation(unsigned kind){if(!mutateCallbacks)return;
#ifdef _M_IX86
 if(kind==1){receiver[0x135]=1;store(0x138,99);}
 if(kind==2){store(0x138,123);receiver[0x13c]=1;receiver[0x13d]=1;}
 if(kind==3){receiver[0x13e]=1;receiver[0x13f]=1;store(0x140,0xabc00055);}
#else
 if(kind==1){liveState->dirty135=1;liveState->value138=99;}
 if(kind==2){liveState->value138=123;liveState->dirty13c=1;liveState->enabled13d=1;}
 if(kind==3){liveState->dirty13e=1;liveState->enabled13f=1;liveState->cached140=0xabc00055;}
#endif
}
'''
s=s[:pos]+helper+s[pos:]
s=s.replace('event(key,value);return E_FAIL;', 'event(key,value);if(key==14)mutation(3);return E_FAIL;')
s=s.replace('event(1,v);}', 'event(1,v);mutation(1);}')
s=s.replace('event(2,v);}', 'event(2,v);mutation(2);}')
s=s.replace('unsigned cases=0;for(unsigned dirty', 'unsigned cases=0;for(bool change:{false,true})for(unsigned dirty')
s=s.replace('receiver.fill(0xa5);', 'mutateCallbacks=change;receiver.fill(0xa5);')
s=s.replace('Services services{state};studioRestoreCommitCandidate', 'liveState=&state;Services services{state};studioRestoreCommitCandidate')
s=s.replace('store(0x140,state.cached140);', 'store(0x140,state.cached140);store(0x138,state.value138);receiver[0x134]=state.dirty134;receiver[0x135]=state.dirty135;receiver[0x13c]=state.dirty13c;receiver[0x13d]=state.enabled13d;receiver[0x13e]=state.dirty13e;receiver[0x13f]=state.enabled13f;')
p.write_text(s)
