import json,re,struct
from pathlib import Path
from pe_image import Image
root=Path(__file__).resolve().parent.parent
inventory=json.loads((root/'outputs/ui-function-recovery-inventory.json').read_text());image=Image(inventory['binary'])
anchors={int(a['va'],16):a for a in inventory['ui_string_anchors'] if re.search(r'clip.?editor|motion.?editor|graph.?editor|context.?menu|keyframe',a['text'],re.I)}
refs=[]
for line in (root/'work/ifm-disassembly.txt').open():
 match=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not match:continue
 raw=bytes.fromhex(match[2]);address=int(match[1],16)
 for offset in range(max(0,len(raw)-3)):
  target=struct.unpack_from('<I',raw,offset)[0]
  if target not in anchors:continue
  at=image.offset(address);assert image.b[at:at+len(raw)]==raw,(hex(address),'stale disassembly')
  refs.append(dict(instruction_va=hex(address),instruction_bytes=raw.hex(),disassembly=match[3].strip(),string_va=hex(target),text=anchors[target]['text'],status='byte-verified instruction reference; function boundaries/semantics unverified'))
report=dict(binary=inventory['binary'],binary_sha256=inventory['sha256'],selected_anchors=len(anchors),references=refs,coverage='Immediate pointer references only; indirect table references and function bodies not covered')
(root/'outputs/editor-ui-code-references.json').write_text(json.dumps(report,indent=2));print(json.dumps(dict(selected_anchors=len(anchors),verified_references=len(refs),sample=refs[:8]),indent=2))
