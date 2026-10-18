import hashlib,json,re,struct
from pathlib import Path
from pe_image import Image
image=Image(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll'))
begin,end=0x104c5750,0x104c58d3;cursor=begin;spans=[]
with Path('work/ifm-disassembly.txt').open() as source:
 previous=None
 for line in source:
  match=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if match:
   if previous:
    address,data=previous;assert address==cursor;at=image.offset(address);assert image.b[at:at+len(data)]==data
    spans.append({'address':hex(address),'bytes':data.hex()});cursor+=len(data);previous=None
   address=int(match[1],16)
   if address>=end:break
   if address>=begin:previous=(address,bytes.fromhex(match[2]))
  elif previous:
   continuation=re.fullmatch(r'\s+((?:[0-9A-F]{2}\s*)+)',line)
   if continuation:previous=(previous[0],previous[1]+bytes.fromhex(continuation[1]))
assert cursor==end,(hex(cursor),hex(end))
report={'binary_sha256':hashlib.sha256(image.b).hexdigest(),'verified_instruction_spans':spans,'world_entry':'104c5870','local_transform_resolution':{'source_handle_offset':'0x6c','interface_global':'10e9a51c','resolve_slot':18,'resolved_method':'104d2860','null_fallback':'104990b0'},'composition_entry':'104c5750','frame_resolution_calls':['104c3ed0','104c3f60'],'frame_hint_strings':{hex(address):image.string(address) for address in [0x10b8e494,0x10b8e488,0x10b8e468,0x10a8a7d4]},'same_frame_branch':{'address':'104c5822','frame_null_fallback':'104990b0','recursive_world_call':'104c5870','compose_call':'10498a60'},'different_frame_branch':{'address':'104c5789','recursive_world_calls':['104c5793','104c57b0'],'frame_null_fallback':'104990b0','rotation_compose_call':'10498a60','translation_source_offsets':['0xc','0x1c','0x2c'],'translation_transform_call':'10498390','translation_write_call':'10498710 with column argument3'},'limits':'Composition branching and call topology verified; matrix helper semantics and frame lookup implementations remain unresolved. Frame hint strings are context, not proof of parent relationships. Separate position/orientation frames must be investigated before substituting one ordinary parent matrix. No runtime execution parity claimed.'}
Path('outputs/sfm64-projected-light-world-transform-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('world_transform_spans='+str(len(spans))+' frame_hints='+str(report['frame_hint_strings']))
