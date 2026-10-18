from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')
lines=[];calls=[];callers=[]
for line in Path('work/studiorender-disassembly.txt').read_text(encoding='utf-8').splitlines():
 match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if not match:continue
 address=int(match[1],16);inside=0x100136F0<=address<0x10013DCE
 caller=bool(re.search(r'\bcall\s+100136F0\b',line))
 if not inside and not caller:continue
 prefix=bytes.fromhex(match[2]);offset=image.offset(address);assert image.b[offset:offset+len(prefix)]==prefix
 if inside:
  lines.append(line)
  if re.search(r'\bcall\b',line):calls.append(line.strip())
 if caller:callers.append(line.strip())
assert image.string(0x100706B8)=='CStudioRender::R_StudioFlexVerts'
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),entry='0x100136f0',exclusive_end='0x10013dce',verified_instruction_prefixes=len(lines)+len(callers),native_label=image.string(0x100706B8),calls=calls,direct_callers=callers,normalization_instruction_sites=[line.strip() for line in lines if re.search(r'\b(?:sqrtss|sqrtps|rsqrtss|rsqrtps|fsqrt)\b',line)],limitations='Offline routine identification by native label reference and instruction prefixes. Branch semantics, accumulation layout, helper behavior, software-vs-hardware dispatch and runtime oracle remain unverified. Wrapped continuation bytes excluded.')
Path('outputs/sfm64-native-software-flex-path.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
Path('outputs/sfm64-native-software-flex-path-disassembly.txt').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print(f'Native R_StudioFlexVerts: {len(lines)} routine prefixes, {len(callers)} direct callers, {len(report["normalization_instruction_sites"])} inline sqrt/rsqrt sites')
print('\n'.join(callers))
