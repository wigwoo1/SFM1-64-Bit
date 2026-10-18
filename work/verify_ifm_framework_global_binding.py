from pathlib import Path
from pe_image import Image
import hashlib,json,struct

image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
pe=image.u32(60);optional=pe+24
reloc_rva=image.u32(optional+96+5*8);reloc_size=image.u32(optional+100+5*8)
targets={0x10E9CCF4,0x10E9CCF8};relocations={}
offset=image.offset(image.base+reloc_rva);end=offset+reloc_size
while offset<end:
    page,size=struct.unpack_from('<II',image.b,offset)
    assert size>=8 and size%2==0 and offset+size<=end
    for entry_offset in range(offset+8,offset+size,2):
        entry=image.u16(entry_offset);address=image.base+page+(entry&0xfff)
        if address in targets:relocations[address]=entry>>12
    offset+=size
assert image.word(0x10E9CCF8)==0x10F4ADA0
assert image.word(0x10E9CCF4)==0x10F4ADA0
assert all(relocations.get(address)==3 for address in targets)
assert image.word(0x10A6A701)==0x10F4ADA0
assert image.word(0x1038F469)==0x10B3939C
assert image.word(0x10B3939C+44)==0x1038F360
assert image.string(0x10B39338)=='VDmElementFrameworkVersion001'
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),
    preferred_image_base=hex(image.base),framework_global='0x10e9ccf8',
    static_pointer_value=hex(image.word(0x10E9CCF8)),
    pointer_relocation_type=relocations[0x10E9CCF8],
    adjacent_global=dict(address='0x10e9ccf4',value=hex(image.word(0x10E9CCF4)),relocation_type=relocations[0x10E9CCF4],interface_role='unverified'),
    initializer='0x10a6a700',constructor='0x1038f460',constructed_vtable='0x10b3939c',
    interface='VDmElementFrameworkVersion001',operate_virtual_offset='0x2c',operate_implementation='0x1038f360',
    conclusion='The on-disk framework global is initialized to the recovered static object, with PE HIGHLOW relocation for rebasing. Once the recovered constructor executes, its vtable slot11 targets the recovered Operate routine. No runtime assignment instruction is required for this initial binding.',
    limitations='Static PE pointer/relocation and existing constructor evidence only. Actual constructor execution, later pointer replacement, live operator scheduling and native parity remain unverified. Installed DLL was read only.')
Path('outputs/sfm64-ifm-framework-global-binding.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified static framework global binding, HIGHLOW relocation and constructor/vtable/Operate chain')
