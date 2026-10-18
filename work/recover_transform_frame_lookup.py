import hashlib,json,re,struct
from pathlib import Path
from pe_image import Image
image=Image(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll'))
def verified(begin,end):
 cursor=begin;spans=[]
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
 return spans
ranges=[(0x104c3ed0,0x104c3fe5),(0x10223170,0x102231f9),(0x10222de0,0x10222e78)]
spans=[{'begin':hex(a),'end':hex(b),'spans':verified(a,b)} for a,b in ranges]
assert [image.word(0x10222e78+4*i) for i in range(4)]==[0x10222e33,0x10222e26,0x10222e19,0x10222e01]
report={'binary_sha256':hashlib.sha256(image.b).hexdigest(),'verified_ranges':spans,'position_lookup':'104c3ed0','rotation_lookup':'104c3f60','override':{'disabled_when_object_byte':'0x64 nonzero','enable_attributes':[image.string(0x10b8e494),image.string(0x10b8e488)],'parent_attribute':image.string(0x10b8e468),'attribute_lookup':'1035dd50','enable_value_accessor':'10058790','reference_resolver':'101e3170 (not fully verified here)','behavior':'If enabled override resolves nonnull, return it; null override falls through to reverse-reference search'},'fallback':{'attribute_name':image.string(0x10a8a7d4),'routine':'10223170','first_reference_interface_slot':115,'next_reference_interface_slot':116,'candidate_filter':'10222de0','filter_arguments':'children name token,0,0','candidate_attribute_interface_slot':89,'candidate_owner_offset':'attribute+8','candidate_owner_type_test':'owner vtable slot4 with token global10eb131c','candidate_name_token_offset':'attribute+0x14','returned_candidate':'first accepted owner in native reference enumeration order'},'filter_mode_jump_table':['10222e33','10222e26','10222e19','10222e01'],'filter_mode0':'skips flag11/flag12 restrictions, checks nonnull owner/type/name token; owner handle relation only when second filter argument nonzero','limits':'Reverse-reference enumeration order, attribute flag meanings, object byte0x64 identity and complete reference/type resolution remain unverified. Do not derive parent by arbitrary first matching graph scan without auditing ambiguity. No runtime selection parity claimed.'}
Path('outputs/sfm64-transform-frame-lookup-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('frame_lookup_verified_spans='+str(sum(len(r['spans']) for r in spans)))
