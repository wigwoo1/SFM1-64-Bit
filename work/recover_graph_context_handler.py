from pathlib import Path
import json,re
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x100d6cf0,0x100d6ef9),(0x100d6fa0,0x100d6fa5)]
groups=[[] for _ in ranges];active=None
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:
  c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
  if c and active is not None: active['bytes']+=bytes.fromhex(c[1]).hex()
  continue
 va=int(m[1],16);active=None
 for i,(lo,hi) in enumerate(ranges):
  if lo<=va<hi: active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());groups[i].append(active);break
 if va>ranges[-1][1]:break
for (lo,hi),spans in zip(ranges,groups):
 cur=lo
 for x in spans:
  va=int(x['va'],16);raw=bytes.fromhex(x['bytes']);assert va==cur and im.b[im.offset(va):im.offset(va)+len(raw)]==raw;cur+=len(raw)
 assert cur==hi
result=dict(binary=r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll',verified_bytes=sum(hi-lo for lo,hi in ranges),handler_va='0x100d6cf0',signal='CQGraphEditorView::OnCustomContextMenuRequested(QPoint)',dispatch='QObject::connect in CQGraphEditorView setup at 0x100d0765 maps customContextMenuRequested to meta-call local id 0; local method thunk 0x100d6fa0 tail-jumps to this body.',findings=['Reads active element from global state field+0x344; absent selection takes cleanup branch.', 'Hit-tests mouse position using editor subobject+0x150, obtains current target/handle through controller/model, and compares with active handle at view+0x14c.', 'When target differs, finalizes/reconciles prior selection, stores target handle and refreshes editor tree/context state; then refreshes view and applies a secondary selected-item action when its gate is enabled.', 'Code references do not expose menu action labels in this handler; actual menu population and action behavior remain delegated to called routines.'],limitations='Context-menu options, enablement predicates, and item-specific action handlers are not yet recovered; avoid claiming full right-click coverage.')
Path('outputs/graph-editor-context-handler-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified graph context handler:',result['verified_bytes'],'bytes')
