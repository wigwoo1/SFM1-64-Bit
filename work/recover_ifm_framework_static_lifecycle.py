from pathlib import Path
from pe_image import Image
import re, json, hashlib, struct

image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1038F460,0x1038F500),(0x10A0E552,0x10A0E5A5),(0x10A6A700,0x10A6A716),(0x10A76510,0x10A76542)]
selected=[];pending=None
def save(item):
    if item is None:return
    address,data,line=item
    if not any(a<=address<b for a,b in ranges):return
    offset=image.offset(address)
    assert image.b[offset:offset+len(data)]==data
    selected.append(dict(address=hex(address),bytes=data.hex(),instruction=line))
with Path('work/ifm-disassembly.txt').open() as stream:
    for line in stream:
        stripped=line.lstrip()
        if len(stripped)>10 and stripped[8:10]==': ':
            address=int(stripped[:8],16)
            if not any(a<=address<b for a,b in ranges):
                save(pending);pending=None
                if address>=0x10A76542:break
                continue
        match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
        if match:
            save(pending);pending=[int(match[1],16),bytes.fromhex(match[2]),line.strip()]
            if pending[0]>=0x10A76542:break
        elif pending:
            tail=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
            if tail:pending[1]+=bytes.fromhex(tail[1])
def references(target):
    needle=struct.pack('<I',target);offset=0;result=[]
    while True:
        offset=image.b.find(needle,offset)
        if offset<0:break
        for rva,size,raw in image.sections:
            if raw<=offset<raw+size:
                result.append(hex(image.base+rva+offset-raw));break
        offset+=1
    return result
assert image.word(0x1038F469)==0x10B3939C
assert image.word(0x10A6A701)==0x10F4ADA0
assert image.word(0x10A6A70B)==0x10A76510
assert image.word(0x10B3939C+44)==0x1038F360
array_begin=array_end=0x10A7F19C
while 0x10001000<=image.word(array_begin-4)<0x10A7E000:array_begin-=4
while 0x10001000<=image.word(array_end+4)<0x10A7E000:array_end+=4
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),
    static_object='0x10f4ada0',initializer='0x10a6a700',constructor='0x1038f460',
    constructor_vtable='0x10b3939c',operate_slot11='0x1038f360',
    graph_subobject='this+4',graph_constructor='0x1039a1c0',
    initialization='Constructor writes framework vtable, constructs graph at +4, allocates 0x400 bytes for handle-vector storage at +0x64, clears count at +0x70 and state fields through +0xb4.',
    cleanup_callback='0x10a76510',cleanup_registration_callee='0x10a0e58e',
    cleanup_subobjects=['this+0xa0','this+0x8c','this+0x78','this+0x64','this+4'],
    cleanup='Four subobjects call 0x10a38ec0; final graph cleanup tail-jumps to 0x1039a260. Registration callee identity is not yet verified.',
    initializer_pointer_references=references(0x10A6A700),
    containing_code_pointer_array=dict(begin=hex(array_begin),last=hex(array_end),count=(array_end-array_begin)//4+1,before=hex(image.word(array_begin-4)),after=hex(image.word(array_end+4)),initializer_index=(0x10A7F19C-array_begin)//4),
    cleanup_registration_wrapper='0x10a0e58e forwards callback to 0x10a0e552 and maps null/non-null return to -1/0. The helper wraps 0x10a0e46b with calls 0x10a1a3c5/0x10a1a3ce and exception machinery. Exact CRT names remain unverified.',
    static_object_address_references=references(0x10F4ADA0),
    verified_complete_instructions=selected,
    limitations='Static initializer/object/lifecycle evidence only. Connection to global 0x10e9ccf8, actual initializer execution, runtime lifetime and scheduling remain unverified; no installed DLL was executed or modified.')
Path('outputs/sfm64-ifm-framework-static-lifecycle.json').write_text(json.dumps(report,indent=2)+'\n')
print('Framework lifecycle byte-checked instructions:',len(selected))
print('Initializer pointer references:',report['initializer_pointer_references'])
