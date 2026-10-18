import json,pathlib,re
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
addresses=[0x10E18C90,0x10E18C8C,0x10E14860,0x10E0B238,0x10E0B26C,0x10E0B234,0x10E0B264,0x10E0B230,0x10E0B27C,0x10B56B04,0x10E18C84,0x10E18C7C,0x10E0B244,0x10E18C74,0x10E18C6C,0x10E18C64]
records=[dict(index=n,name=im.string(a),name_address=hex(a),branch=hex(im.word(0x10A5ED88+4*n)),arity=1) for n,a in enumerate(addresses)]
for arity,parser,table,names in [
 (2,0x10A5EDD0,0x10A5EF5C,[0x10B3A2B0,0x10B3A2AC,0x10E14868,0x10E0B280]),
 (3,0x10A5EF70,0x10A5F2C0,[0x10E18CBC,0x10E17784,0x10E18CB4,0x10E18CAC,0x10E18CA4,0x10E18C9C,0x10E18C94,0x10AD8D38]),
 (5,0x10A5F2E0,None,[0x10E18CD0,0x10E18CC4])]:
 for n,a in enumerate(names):
  records.append(dict(index=n,name=im.string(a),name_address=hex(a),branch=hex(im.word(table+4*n)) if table else hex([0x10A5F4E5,0x10A5F478][n]),arity=arity,parser=hex(parser)))
out=pathlib.Path('outputs/sfm-expression-functions.json')
out.write_text(json.dumps({'source':'installed bin/tools/ifm.dll','parser':'0x10A5EA20','jump_table':'0x10A5ED88','functions':records,'status':'Names and dispatch recovered; not all function implementations verified'},indent=2))
print(json.dumps(records,indent=2))
