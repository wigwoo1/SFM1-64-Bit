from pathlib import Path
from pe_image import Image
import hashlib,json
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
assert image.string(0x10ACF9F4)=='result'
assert image.string(0x10AE4ABC)=='expr'
expected={0x104CB22F:'8dbeb4000000',0x104CB235:'576a0368f4f9ac10e89e63e9ff894704',0x104CB3A9:'e8324e5900',0x104CB3AE:'8b8bb80000008d45fc50e8831ceaff',0x104CB3F6:'5b8be55dc3'}
for address,data in expected.items():
 offset=image.offset(address);assert image.b[offset:offset+len(bytes.fromhex(data))]==bytes.fromhex(data)
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),operator='CDmeExpressionOperator',operate='0x104cb2b0',result_binding='0x104cb235: type3 result storage at object+0xb4; attribute pointer at object+0xb8',evaluate_call='0x104cb3a9 -> 0x10a601e0',result_write_call='0x104cb3b8 -> 0x1036d040, using object+0xb8 and evaluated local float',checked_bytes={hex(k):v for k,v in expected.items()},implication='Native Operate uses the ordinary scalar float setter for result. Owned result commits use the recovered gate, storage conversion and owner dirty path.',limits='Static selected instruction spans. No live native scheduler feedback verification; constructor attribute defaults, derived callbacks and post-write queue lifecycle remain incomplete.')
Path('outputs/sfm64-ifm-expression-result-write.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified native expression result binding and scalar setter call')
