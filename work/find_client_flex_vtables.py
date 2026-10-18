from pathlib import Path
from pe_image import Image
import re,struct,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll')
words={}
for rva,size,raw in image.sections:
    for offset in range(raw,raw+size-3,4):
        value=image.u32(offset)
        words.setdefault(value,[]).append(image.base+rva+offset-raw)
classes=[]
for match in re.finditer(rb'\.\?AV[^\x00]*BaseFlex[^\x00]*\x00',image.b):
    raw=match.start()
    address=next(image.base+rva+raw-start for rva,size,start in image.sections if start<=raw<start+size)
    descriptor=address-8;tables=[]
    for field in words.get(descriptor,[]):
        locator=field-12
        try:
            if image.word(locator)!=0 or image.word(locator+8)!=0:continue
            for pointer in words.get(locator,[]):
                table=pointer+4
                entries=[image.word(table+4*slot) for slot in range(40)]
                if not all(any(rva<=entry-image.base<rva+size for rva,size,_ in image.sections) for entry in entries):continue
                tables.append(dict(vtable=hex(table),locator=hex(locator),object_offset=image.word(locator+4),slot14=hex(entries[14]),slot39=hex(entries[39]),entries=[hex(entry) for entry in entries]))
        except ValueError:continue
    classes.append(dict(name=match[0][:-1].decode('ascii'),type_descriptor=hex(descriptor),tables=tables))
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),classes=classes,limitations='Offline PE32 MSVC RTTI candidate scan. Section-backed pointers alone do not prove code entries, callback identity, or engine object usage.')
Path('outputs/sfm64-client-baseflex-vtables.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
for cls in classes:
    print(cls['name'])
    for table in cls['tables']:print('table',table['vtable'],'object offset',table['object_offset'],'slot14',table['slot14'],'slot39',table['slot39'])
