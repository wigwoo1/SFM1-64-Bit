from pathlib import Path
import json,re,struct,hashlib
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
inv=json.loads(Path('outputs/ui-function-recovery-inventory.json').read_text());classes=[a for a in inv['ui_string_anchors'] if re.fullmatch(r'CQ\w*(?:GraphEditor|MotionEditor|ClipEditor|Timeline|Timestrip|SFMMainWindow|SFMController)\w*',a['text'])]
for class_name,string_va in (('CQSFMMainWindow','0x10AA13A0'),('CQSFMController','0x10AA0958')):
 if not any(a['text']==class_name for a in classes):classes.append({'text':class_name,'va':string_va})
objects=[]
for anchor in classes:
 target=int(anchor['va'],16);needle=struct.pack('<I',target);pos=0
 while True:
  pos=im.b.find(needle,pos)
  if pos<0:break
  offset=pos;pos+=1
  section=next(((rva,size,raw) for rva,size,raw in im.sections if raw<=offset<raw+size),None)
  if not section:continue
  rva,size,raw=section;va=im.base+rva+offset-raw
  try:
   data=im.word(va+4);header=[im.word(data+i*4) for i in range(14)]
   if header[0] not in (5,6) or header[1]!=0 or header[4]>512 or header[5]!=14:continue
   methods=[]
   for local_id in range(header[4]):
    row=[im.word(data+(header[5]+local_id*5+i)*4) for i in range(5)]
    if max(row[:4])>65536:raise ValueError('offset limit')
    methods.append(dict(local_id=local_id,signature=im.string(target+row[0]),parameters=im.string(target+row[1]),return_type=im.string(target+row[2]),flags=row[4]))
   objects.append(dict(class_name=anchor['text'],metaobject_va=hex(va-4),string_data=hex(target),method_data=hex(data),metadata_revision=header[0],methods=methods))
  except (ValueError,UnicodeError,IndexError,struct.error):continue
assert any(o['class_name']=='CQGraphEditorWindow' and len(o['methods'])==6 for o in objects)
assert any(o['class_name']=='CQSFMController' and any(m['signature']=='OnShowMotionEditorWidget()' for m in o['methods']) for o in objects)
report=dict(binary_sha256=hashlib.sha256(im.b).hexdigest(),objects=objects,scope='Installed metadata signature inventory; selected graph, timeline, main-window menu and editor-window action dispatch paths have separate byte-verified audits')
Path('outputs/editor-qt-method-inventory.json').write_text(json.dumps(report,indent=2))
print(json.dumps(dict(classes=len(objects),methods=sum(len(o['methods']) for o in objects),per_class=[(o['class_name'],len(o['methods'])) for o in objects]),indent=2))
