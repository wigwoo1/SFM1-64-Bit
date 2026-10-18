import hashlib,json,re,struct
from pathlib import Path
from pe_image import Image
image=Image(Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll'))
begin,end=0x10723680,0x10723a0f;cursor=begin;spans=[]
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
registration=json.loads(Path('outputs/sfm64-projected-light-field-registration-recovery.json').read_text())
by_offset={int(f['member_wrapper_offset'],16):f['name'] for f in registration['fields']}
pairs=[(0x154,0x4c),(0x15c,0x8c),(0xe8,0x5c),(0xd0,0x1c),(0x108,0x80),(0x110,0x7c),(0x118,0x78),(0x120,0x84),(0x144,0x88),(0x16c,0x98),(0x174,0x9c),(0x17c,0xa0),(0x184,0xa4),(0x18c,0xb0),(0x194,0xb4),(0x19c,0xb8),(0x1a4,0xbc),(0x1ac,0xc0),(0x1bc,0x134),(0x1c4,0x138),(0x1d4,0x140),(0x1dc,0x144)]
report={'binary_sha256':hashlib.sha256(image.b).hexdigest(),'routine':'10723680','verified_instruction_spans':spans,'float_copies':[{'name':by_offset[src],'cached_value_offset':hex(src),'destination_offset':hex(dst),'operation':'x87 FLD/FSTP; masked signaling NaNs quiet'} for src,dst in pairs],'fov':{'source_offsets':['0xd8','0xe0'],'destination_offsets':['0x24','0x28'],'operation':'COMISS horizontal,vertical; retain horizontal only on JA, otherwise vertical (including unordered)'},'color':{'source_rgb_bytes':'0xa0..0xa2','intensity_offset':'0xa8','scale_constant_word':hex(image.word(0x10ac2a30)),'operation':'each channel: float(byte) * intensity * scale, sequential MULSS','destination_rgb':'0x50..0x58'},'transform_calls':['104c5870','1049d180'],'jitter':{'source_vector_offsets':['0x128','0x12c'],'radius_offset':'0x100','basis_helpers':['1049e160','10498be0'],'operation':'conditional two basis offsets scaled by jitter components then radius and added sequentially to origin'},'texture':{'source_pointer':'0x1e4','destination_pointer':'0x64','frame_offset':'0x6c','frame_operation':'signed animationTime * scale * frameRate, CVTTSS2SI then signed remainder by texture slot5 result; zero when no texture','time_scale_word':hex(image.word(0x10a8dfd4))},'other_copies':[{'source':'0x14c','destination':'0x71','name':'drawShadowFrustum'},{'source':'0xf8','destination':'0x70','name':'castsShadows'},{'source':'0x1b4','destination':'0x130','name':'volumetric'},{'source':'0x1cc','destination':'0x13c','name':'numPlanes'}],'uberlight_flag':'destination byte0x95 bit3 replaced with source byte0x164 bit0','zeros':['0xa8','0xac'],'limits':'Offline instruction evidence only. Original x86 output pointer layout is not proposed x64 ABI. Transform helpers, inherited source field identities, live animation, texture lifetime and complete renderer integration remain unverified.'}
Path('outputs/sfm64-projected-light-state-conversion-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('projected_light_conversion_spans='+str(len(spans))+' float_copies='+str(len(pairs)))
