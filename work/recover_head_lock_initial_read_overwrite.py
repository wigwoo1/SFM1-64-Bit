import re,json
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=0x1009bb6d:break
 if not(0x1009b311<=va<0x1009b353 or 0x1009bb40<=va<0x1009bb47 or 0x1009bb50<=va<0x1009bb6d):continue
 raw=bytes.fromhex(m[2]);at=im.offset(va);assert im.b[at:at+len(raw)]==raw;spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
r=dict(initial_read='QSettings::value(headIsLocked,QVariant(true)).toBool() -> bytef4; direct store without setter',overwrite='bytef5 getter1009bb40/setter1009bb50; changed values store then emit10079170; unchanged early exit; no persistence in this setter',verified_spans=spans,imports=json.loads(Path('outputs/head-lock-settings-read-imports.json').read_text()),limitations='Read enclosing group/backend and overwrite initialization pending; candidate not wired into native UI.')
Path('outputs/head-lock-initial-read-overwrite-recovery.json').write_text(json.dumps(r,indent=2))
Path('work/timeline_head_lock_state_candidate.h').write_text('''#pragma once
// Recovered state transitions; settings and UI adapters are supplied by the application.
struct TimelineHeadLockStateCandidate {
 bool locked,overwrite;
 explicit TimelineHeadLockStateCandidate(bool savedLock,bool initialOverwrite):locked(savedLock),overwrite(initialOverwrite){}
 static constexpr bool missingSavedLockDefault=true;
 bool buttonChecked()const{return locked;}
 bool buttonEnabled()const{return !overwrite;}
 template<class Persist,class Notify>void setLocked(bool value,Persist persist,Notify notify){if(locked==value)return;locked=value;persist(value);notify();}
 template<class Notify>void setOverwrite(bool value,Notify notify){if(overwrite==value)return;overwrite=value;notify();}
};
''')
Path('work/compile_graph_editor_keymode_state.cpp').write_text('#include "graph_editor_keymode_state_candidate.h"\n#include "timeline_head_lock_state_candidate.h"\n')
print('Initial read and overwrite spans verified:',len(spans))
