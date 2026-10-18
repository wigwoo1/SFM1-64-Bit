from pathlib import Path
from pe_image import Image
import json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
expected={0x10363353:'8b4514568bf18b4d08894604',0x10363386:'8b450c8bc84966894610',0x1036339D:'740583e90e75090d0004000066894610',0x103633C6:'b80002000066094610',0x1036506A:'e8e1e2ffff',0x10361670:'e8cb390000',0x104CB235:'576a0368f4f9ac10e89e63e9ff894704'}
for address,data in expected.items():
 offset=image.offset(address);assert image.b[offset:offset+len(bytes.fromhex(data))]==bytes.fromhex(data)
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),external_attribute_constructor='0x10363350',allocation_wrapper='0x10365040',element_binding='0x103615e0',initial_flags='uint16(type) | 0x0200 | (type==1 or type==15 ? 0x0400 : 0)',result_initial_flags='0x0203 for CDmeExpressionOperator result external storage at operator+0xb4',checked_bytes={hex(k):v for k,v in expected.items()},limits='Constructor initial flag writes only. Subsequent attachment/notification callbacks and live class defaults not verified. Owned graph payloads deliberately do not use external-storage bit0x0200.')
Path('outputs/sfm64-ifm-external-attribute-defaults.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified external attribute constructor initial flags and expression result binding')
