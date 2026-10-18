import json,pathlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
table=0x10A1E583
result={'source':'installed bin/tools/ifm.dll','entry':'0x10A103F1','locale_wrapper':'0x10A102CE','conversion_wrapper':'0x10A1DD78','state_parser':'0x10A1DED7','state_table':hex(table),'states':[{'state':n,'target':hex(im.word(table+4*n))} for n in range(12)],'digit_accumulation':'0x10A320F8','extended_to_double':'0x10A31655','packing_configuration':{hex(a):im.word(a) for a in [0x10F22860,0x10F22864,0x10F22868,0x10F2286C]},'status':'dispatch and packing format verified; complete numeric rounding unverified'}
pathlib.Path('outputs/sfm-expression-numeric-dispatch.json').write_text(json.dumps(result,indent=2))
print('Recovered 12 numeric parser dispatch entries')
