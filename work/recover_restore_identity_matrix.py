from pathlib import Path
import hashlib,json,struct,re
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
begin,end=0x100d5490,0x100d54bd
raw=im.b[im.offset(begin):im.offset(end)]
expected=bytes.fromhex('558bec568b75086a306a0056e8ff9e0000f30f1005f8a50f1083c40cf30f1106f30f114614f30f1146285e5dc3')
assert raw==expected and len(raw)==45
assert begin+17+struct.unpack('<i',raw[13:17])[0]==0x100df3a0
assert im.word(0x100fa5f8)==0x3f800000
source=json.loads(Path('outputs/sfm64-full-restore-dependency-audit.json').read_text());assert source['sha256']==sha
calls=[call for call in source['direct_calls'] if call['target']==hex(begin)];assert len(calls)==1
Path('outputs/restore-identity-matrix-original.bin').write_bytes(raw)
out={'sha256':sha,'begin':hex(begin),'end':hex(end),'bytes':45,'fill_call':{'offset':12,'target':'0x100df3a0','arguments':['destination','0','48']},'constant':{'address':'0x100fa5f8','bits':'0x3f800000'},'writes':['destination+0','destination+20','destination+40'],'fullowner_callsite':calls,'interpretation':'Initialize a 3x4 float matrix to identity by zeroing48bytes then storing1 at diagonal indices0/5/10.','limits':'Exact bytes/constant/callsite checked; native combined fill execution and fullowner binding pending.'}
Path('outputs/sfm64-restore-identity-matrix-recovery.json').write_text(json.dumps(out,indent=2)+'\n')
print('restore_identity_matrix_bytes=45 constant=1.0 verified')
