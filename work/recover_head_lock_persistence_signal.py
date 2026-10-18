import json
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');meta=0x10aa1024;strings=im.word(meta+4);data=im.word(meta+8);count=im.word(data+16);table=im.word(data+20)
signature=im.string(strings+im.word(data+table*4))
raw=bytes.fromhex('6a006a00682410aa1051ff1570b2a71083c410c3');assert im.b[im.offset(0x10079150):im.offset(0x10079150)+len(raw)]==raw
r=dict(class_name=im.string(strings),change_signal=signature,signal_local_id=0,signal_entry='0x10079150',verified_signal_bytes=raw.hex(),persistence=dict(api='Qt4 QSettings',group_template='%1_%2',group_object='receiver member+8 QObject::objectName()',group_numeric_suffix=9,key='headIsLocked',value_type='QVariant(bool)',sequence=['store flag','begin group','setValue','endGroup','emit local signal0','destroy settings']),imports=json.loads(Path('outputs/head-lock-persistence-imports.json').read_text()),limitations='Settings backend/app identifiers, initial read path, signal receivers and checkbox/playback effects pending.')
Path('outputs/timeline-head-lock-persistence-signal-recovery.json').write_text(json.dumps(r,indent=2));print(json.dumps(dict(class_name=r['class_name'],signal=signature,persistence=r['persistence']),indent=2))
