from pathlib import Path
import json,hashlib,struct
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
audit=json.loads(Path('outputs/sfm64-owner-reset-cleanup-recovery.json').read_text());assert audit['sha256']==sha
rows=[r for r in audit['instructions'] if 0x10020dd7<=int(r['address'],16)<0x10020e1c]
cursor=0x10020dd7
for r in rows:
 a=int(r['address'],16);b=bytes.fromhex(r['bytes']);assert a==cursor and im.b[im.offset(a):im.offset(a)+len(b)]==b;cursor+=len(b)
assert cursor==0x10020e1c and rows[0]['bytes']=='33c0' and rows[1]['bytes']=='8bcb'
for r in rows[2:-1]:
 b=bytes.fromhex(r['bytes']);assert b[:2] in (b'\x89\x83',b'\x88\x83') and len(b)==6
assert bytes.fromhex(rows[-1]['bytes'])[0]==0xe8
assert 0x10020e1c+struct.unpack('<i',bytes.fromhex(rows[-1]['bytes'])[1:])[0]==0x10019b80
cookie=im.b[im.offset(0x100df5ad):im.offset(0x100df5bc)]
assert cookie==bytes.fromhex('3b0d70c317107502f3c3e9207e0000')
assert 0x100df5bc+struct.unpack('<i',cookie[11:])[0]==0x100e73dc
prologue=im.b[im.offset(0x1001fb96):im.offset(0x1001fba0)]
assert prologue==bytes.fromhex('a170c3171033c58945fc')
epilogue=im.b[im.offset(0x100211d4):im.offset(0x100211e1)]
assert epilogue==bytes.fromhex('8b4dfc5f5e33cd5be8cc e30b00'.replace(' ',''))
report={'sha256':sha,'resource_callsite':rows,'receiver':'Full restore captures ECX in EBX at1001fba2; call10020e17 sets ECX=EBX at10020dd9 then only stores before call. Resource helper receives same owner, no subobject adjustment.',
 'field_mapping':{'owner648':'queue/device state, pointer at648 and flush subobject beginning648','owner668':'resource pointer array; slots0/4/8 widen to pointer elements0/1/2','owner680':'final resource pointer','owner40/44':'DWORD multisample arguments'},
 'cookie':{'address':'0x100df5ad','bytes':cookie.hex(),'global':'0x1017c370','failure_target':'0x100e73dc','prologue':prologue.hex(),'epilogue':epilogue.hex(),'classification':'Compiler stack-cookie check: saved global XOR frame pointer reversed in epilogue, compare ECX to global; equal returns, mismatch tails to failure handler.'},
 'limits':'Byte-verified static binding only; whole restore not executed. X64 policy requires pointer widening and explicit queue/device services. Compiler-generated x64 stack protection replaces PE32 cookie machinery; failure handler behavior not executed.'}
Path('outputs/sfm64-full-restore-resource-binding.json').write_text(json.dumps(report,indent=2)+'\n');print('full_restore_resource_receiver_and_cookie_verified')
