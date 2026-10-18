import json,pathlib,struct
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
entries=[]
for sign,base in [(1,0x10F22888),(-1,0x10F229E8)]:
 for group in range(5):
  for digit in range(1,8 if group<4 else 2):
   va=base+group*0x54+(digit-1)*12
   words=[im.word(va+4*n) for n in range(3)]
   entries.append({'decimal_exponent':sign*digit*8**group,'address':hex(va),'words':[hex(w) for w in words],'binary_exponent':(words[2]>>16)&0x7fff,'low_word_adjustment':(words[0]&0xffff)>=0x8000})
pathlib.Path('outputs/sfm-decimal-scaling-tables.json').write_text(json.dumps({'source':'installed bin/tools/ifm.dll','entries':entries,'status':'58 table entries recovered; extended multiplication rounding pending'},indent=2))
print('Recovered',len(entries),'decimal scaling constants')
rows=[]
for e in entries:
 words=[int(w,16) for w in e['words']]
 limbs=[(words[n//2]>>(16*(n%2)))&65535 for n in range(5)]
 rows.append('{'+e['address']+'u,{{'+','.join(map(str,limbs))+'},'+str(words[2]>>16)+'}}')
pathlib.Path('work/decimal_scaling_constants.inc').write_text(',\n'.join(rows))
