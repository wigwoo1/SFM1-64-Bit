import hashlib,json,re,struct
from pathlib import Path
from pe_image import Image
image=Image(Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll'))
begin,end=0x10722fd0,0x10723650;cursor=begin;spans=[]
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
fields=[]
for member,nameAddress,defaultAddress in [(0xd0,0x10ae6298,0x10a8af84),(0xd8,0x10aad6b8,0x10a8a86c),(0xe0,0x10aad6ac,0x10a8a86c),(0xe8,0x10ae6238,0x10ac4640),(0x100,0x10ae6534,None),(0x108,0x10ae61bc,0x10bc2080),(0x110,0x10ae6200,0x10a8baa4),(0x118,0x10ae61cc,0x10a8c508),(0x144,0x10ae61f4,0x10a8a56c),(0x120,0x10ae5874,0x10a8a56c),(0x13c,0x10acfb18,0x10ab2848),(0x154,0x10bc2074,0x10b8d008),(0x15c,0x10bc2060,0x10a8a56c),(0x16c,0x10ae6194,0x10a8baa4),(0x174,0x10ae618c,0x10a8f53c)]:
 bits=image.word(defaultAddress) if defaultAddress is not None else 0
 fields.append({'name':image.string(nameAddress),'member_wrapper_offset':hex(member),'attribute_pointer_offset':hex(member+4),'type_argument':3,'default_word':hex(bits),'default_float':struct.unpack('<f',struct.pack('<I',bits))[0]})
lifecycle_begin=0x10723d90
lifecycle_bytes=bytes.fromhex('56 8b f1 e8 c8 2e fb ff 8b ce e8 31 ec d9 ff 8b ce e8 8a d6 e9 ff 8b ce e8 f3 d8 e9 ff 8b ce 5e e9 1b f2 ff ff')
at=image.offset(lifecycle_begin)
assert image.b[at:at+len(lifecycle_bytes)]==lifecycle_bytes
vtable=0x10bc20c4
assert image.word(vtable+10*4)==lifecycle_begin
assert lifecycle_begin+len(lifecycle_bytes)+struct.unpack('<i',lifecycle_bytes[-4:])[0]==begin
lifecycle={'vtable':hex(vtable),'slot':10,'entry':hex(lifecycle_begin),'bytes':lifecycle_bytes.hex(),'preceding_call_targets':['106d6c60','104c29d0','105c1430','105c16a0'],'tail_target':hex(begin),'this_preserved':True,'semantic_name':'unnamed lifecycle entry; field registration linkage verified'}
boolean_fields=[{'name':image.string(name),'member_wrapper_offset':hex(member),'attribute_pointer_offset':hex(member+4),'type_argument':4,'default':default,'setter':'10366260'} for member,name,default in [(0xf8,0x10ae621c,True),(0x14c,0x10ae61e0,False),(0x164,0x10ae61a0,False)]]
report={'binary_sha256':hashlib.sha256(image.b).hexdigest(),'routine':'10722fd0','fields':fields,'boolean_fields':boolean_fields,'other_registrations':[{'name':image.string(0x10abd744),'member_wrapper_offset':'0xf0','type_argument':5,'default':'runtime interface global10e9a51c slot50; not evaluated','attribute_flag_or':'0x100'},{'name':image.string(0x10ae582c),'member_wrapper_offset':'0x134','type_argument':7,'default_word':'0x0','setter':'10366370'}],'attribute_flag_changes':[{'wrapper':'0x120','attribute_flag_or':'0x8000'}],'creation_helper':'103615e0 with name,type,member wrapper pointer; returned attribute pointer stored wrapper+4','default_setter':'1036d040 with pointer to local default float','verified_instruction_spans':spans,'lifecycle_linkage':lifecycle,'limits':'Partial registrations through wrapper0x174 only. Wrapper offsets contain attribute pointers, not inline light float values or proposed x64 ABI. Lifecycle entry naming, getter semantics, runtime animation and projected-light rendering remain unverified. Attribute flag meanings and runtime texture default remain unresolved.'}
for member,nameAddress,defaultAddress in [(0x17c,0x10ae6184,0x10a8af84),(0x184,0x10ae617c,0x10ae3904),(0x18c,0x10ac25fc,0x10ad9568),(0x194,0x10ae6170,0x10a8dfd8),(0x19c,0x10ac25f4,0x10ad9568),(0x1a4,0x10ae6164,0x10a8dfd8),(0x1ac,0x10a9448c,0x10ae3900),(0x1dc,0x10ae6150,0x10a8a56c),(0x1bc,0x10ae61ac,0x10ae3900),(0x1c4,0x10ae5864,None),(0x1d4,0x10ae583c,None)]:
 bits=image.word(defaultAddress) if defaultAddress is not None else 0
 fields.append({'name':image.string(nameAddress),'member_wrapper_offset':hex(member),'attribute_pointer_offset':hex(member+4),'type_argument':3,'default_word':hex(bits),'default_float':struct.unpack('<f',struct.pack('<I',bits))[0]})
boolean_fields.append({'name':image.string(0x10bc2054),'member_wrapper_offset':'0x1b4','attribute_pointer_offset':'0x1b8','type_argument':4,'default':False,'setter':'10366260'})
report['other_registrations'].extend([{'name':image.string(0x10ae5858),'member_wrapper_offset':'0x1cc','attribute_pointer_offset':'0x1d0','type_argument':2,'default_word':'0x80','setter':'1036cef0'},{'name':image.string(0x10ae5848),'member_wrapper_offset':'0x128','attribute_pointer_offset':'0x130','type_argument':9,'default_words':['0x0','0x0'],'setter':'10371d10'}])
report['attribute_flag_changes'].extend({'wrapper':wrapper,'attribute_flag_or':'0x8000'} for wrapper in ['0x1c4','0x1cc','0x1d4','0x128'])
report['limits']='Complete local registration routine verified through RET at1072364f; inherited fields are registered by preceding lifecycle calls and not covered here. Wrapper offsets are original x86 attribute wrappers, not proposed x64 ABI. Runtime getters, animation, projected-light conversion and attribute flag meanings remain unresolved; texture default requires runtime interface.'
Path('outputs/sfm64-projected-light-field-registration-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('projected_light_registered_fields='+str([(field['name'],field['default_float']) for field in fields]))
