from pathlib import Path
import re,json
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');labels=[]
for mode in range(4):
 target=im.word(0x100d4ec8+mode*4);assert im.b[im.offset(target)]==0x68
 label=im.string(im.word(target+1));labels.append(dict(mode=mode,label=label,branch=hex(target)))
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=0x100d4ec6:break
 if va<0x100d4e40:continue
 raw=bytes.fromhex(m[2]);at=im.offset(va);assert im.b[at:at+len(raw)]==raw
 spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
r=dict(entry='0x100d4e40',mode_label_mapping=labels,unknown_mode_label=im.string(0x10a8a4f0),text_target_member='0x94',text_setter_iat='0x10a7cd64',verified_spans=spans,limitations='Label refresh and enum mapping byte verified; text setter import and callback invocation require further audit.')
Path('outputs/graph-editor-keymode-label-refresh-recovery.json').write_text(json.dumps(r,indent=2))
Path('work/graph_editor_keymode_state_candidate.h').write_text('''#pragma once
#include <array>
#include <cstdint>
#include <string_view>
// Recovered label/check state only; no interpolation or key-editing implementation.
struct GraphEditorKeyModeStateCandidate {
 int32_t mode=0;
 static constexpr std::array<std::string_view,4> labels{"Curve Fit","Linear","Spline","Stepped"};
 void setMode(int32_t value){mode=value;} // Original setter stores without a range guard.
 std::array<bool,4> checked()const{return {mode==0,mode==1,mode==2,mode==3};}
 std::string_view label()const{return uint32_t(mode)<labels.size()?labels[size_t(mode)]:std::string_view{};}
};
''')
print(json.dumps(dict(labels=labels,unknown_mode_label=r['unknown_mode_label'],verified_spans=len(spans)),indent=2))
