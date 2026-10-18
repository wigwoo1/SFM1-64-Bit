from pathlib import Path
import json,struct
exec(Path('work/recover_tracker_material_system_factory.py').read_text().split('connect=region')[0])
linker=region(0x1006fd60,0x1006fd82);lookup=region(0x1006fd90,0x1006fdfe)
registrations=[]
for i,row in enumerate(rows):
 raw=bytes.fromhex(row['bytes']);a=int(row['address'],16)
 if raw[0]!=0xe8 or a+5+struct.unpack_from('<i',raw,1)[0]!=0x1006fd60:continue
 context=rows[i-3:i+2];chunks=[bytes.fromhex(x['bytes']) for x in context]
 assert [x[0] for x in chunks]==[0x68,0x68,0xb9,0xe8,0xc3]
 body=region(int(context[0]['address'],16),int(context[-1]['address'],16)+1)
 name=struct.unpack_from('<I',chunks[0],1)[0];factory=struct.unpack_from('<I',chunks[1],1)[0];node=struct.unpack_from('<I',chunks[2],1)[0]
 registrations.append(dict(name=im.string(name),factory=hex(factory),node=hex(node),body=body))
opt=im.u32(60)+24;cell=0x10108960;classification=None
for i in range(im.u16(im.u32(60)+6)):
 o=opt+im.u16(im.u32(60)+20)+40*i;virtual_size=im.u32(o+8);rva=im.u32(o+12);raw_size=im.u32(o+16)
 if rva<=cell-im.base<rva+virtual_size:
  classification=dict(section=im.b[o:o+8].split(b'\0')[0].decode(),virtual_size=virtual_size,raw_size=raw_size,relative_offset=cell-im.base-rva,zero_fill_tail=cell-im.base-rva>=raw_size)
assert classification and classification['zero_fill_tail']
references=[x for x in rows if '10108960h' in x['instruction']]
for x in references:
 a=int(x['address'],16);b=bytes.fromhex(x['bytes']);assert im.b[im.offset(a):im.offset(a)+len(b)]==b
report=dict(sha256=sha,linker=linker,lookup=lookup,verified_direct_registration_stubs=registrations,tracker_registered_in_stubs=any(x['name']=='VBAllocTracker001' for x in registrations),upstream_cell_initial_storage=classification,verified_exact_cell_references=references,
finding='Upstreamfactory cell isinsidePEdata virtualzero-filltail, initializednull onimage mapping. Localregistry linkedlookup isexactcase-sensitive/firstmatch; directstaticregistrationstubs listedwithfullbodybytes. No listedstub registersVBAllocTracker001.',
limits='Image initialzero doesnotproveruntimevalueunchanged. Exactreadinventory/stubinventory excludescomputedaliases/externalwrites/runtimeaddednodes/indirectregistrationcalls; cannotclaimtrackerabsenceatruntime.',
next_action='Compose materialwrapper/localregistry withshaderfallback inoriginaloracle; trace aliases orlivefactoryvaluebeforeclaimingselectedruntimeprovider.')
Path('outputs/sfm64-material-tracker-registry.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_material_registry_bytes='+str(linker['bytes']+lookup['bytes'])+' registration_stubs='+str(len(registrations))+' upstream_initial_zero=True tracker_in_stubs='+str(report['tracker_registered_in_stubs']))
