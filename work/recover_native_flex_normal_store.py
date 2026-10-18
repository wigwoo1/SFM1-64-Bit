from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')
ranges=[(0x10012160,0x100123B4),(0x10012630,0x10012678),(0x10009D7E,0x10009D93),(0x10001090,0x100010EC)]
instructions=[]
for line in Path('work/studiorender-disassembly.txt').read_text(encoding='utf-8').splitlines():
 match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if not match:continue
 address=int(match[1],16)
 if not any(a<=address<b for a,b in ranges):continue
 prefix=bytes.fromhex(match[2]);offset=image.offset(address)
 assert image.b[offset:offset+len(prefix)]==prefix
 instructions.append((address,line))
assert any(a==0x1001238D and 'movss' in line and '[eax+18h]' in line for a,line in instructions)
assert any(a==0x10012392 and 'movss' in line and '[eax+1Ch]' in line for a,line in instructions)
assert any(a==0x10012671 and '10012160' in line for a,line in instructions)
accumulator=[line for a,line in instructions if 0x10012160<=a<0x100123B4]
assert not any(re.search(r'\b(?:sqrtss|sqrtps|rsqrtss|rsqrtps|fsqrt)\b',line) for line in accumulator)
calls=[line.strip() for line in accumulator if re.search(r'\bcall\b',line)]
helper=[line for a,line in instructions if 0x10001090<=a<0x100010EC]
assert not any(re.search(r'\b(?:sqrtss|sqrtps|rsqrtss|rsqrtps|fsqrt|call)\b',line) for line in helper)
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),verified_instruction_prefixes=len(instructions),ranges=[dict(first=hex(a),exclusive_end=hex(b)) for a,b in ranges],accumulator_calls=calls,observations=['Normal flex accumulator ends with component stores, loop continuation and return; no inline sqrt/rsqrt instructions occur inside the extracted range. A call to 10001090 remains to be analyzed.','Wrapper calls accumulator then returns.','Enclosing flex loop exit performs an indirect call through 100700F0 before returning; this call has not been resolved.'],limitations='No runtime oracle; no proof about downstream normal/tangent processing, call semantics or other execution paths. Instruction-prefix verification excludes wrapped continuation bytes.')
Path('outputs/sfm64-native-flex-normal-store-disassembly.txt').write_text('\n'.join(line for a,line in instructions)+'\n',encoding='utf-8')
report['helper_observation']='10001090 uses integer count/index operations, writes two 16-bit slot fields, increments owner+300034 and returns owner plus a 32-byte-stride slot address. No calls or sqrt/rsqrt instructions occur in this extracted helper. Allocation/indexing interpretation is an inference; downstream normalization remains unresolved.'
resolution=Path('outputs/sfm64-flex-exit-import-resolution.json')
if resolution.exists():
 resolved=json.loads(resolution.read_text(encoding='utf-8'))
 assert resolved['binary_sha256']==report['binary_sha256']
 symbols={entry['iat_address']:entry['symbol'] for entry in resolved['resolved_imports']}
 assert symbols['0x100700f0']=='?ExitScope@CVProfile@@QAEXXZ' and symbols['0x100700f4']=='g_VProfCurrentProfile'
 report['observations'][0]='Normal flex accumulator ends with component stores, loop continuation and return; no inline sqrt/rsqrt instructions occur in the extracted range. Its helper 10001090 is separately extracted below.'
 report['observations'][2]='Enclosing flex loop exit calls the PE import tier0.dll CVProfile::ExitScope through 100700F0 with g_VProfCurrentProfile from 100700F4. Profiling bookkeeping interpretation is supported by import names; native runtime behavior not executed.'
Path('outputs/sfm64-native-flex-normal-store-recovery.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(f'Native flex normal-store evidence saved: {len(instructions)} instruction prefixes verified; downstream normalization unresolved')
