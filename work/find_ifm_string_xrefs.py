import sys, struct
sys.path.insert(0, 'work')
from pe_image import Image
x=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
print('base',hex(x.base),'sections',[(hex(a),hex(b),hex(c)) for a,b,c in x.sections])
for target in [0x10AA191C,0x10AB6564,0x10AB6584,0x10AA18A0,0x10AA18C2]:
    needle=struct.pack('<I',target)
    hits=[]
    for rva,size,raw in x.sections:
        stop=raw+size
        pos=raw
        while True:
            pos=x.b.find(needle,pos,stop)
            if pos<0: break
            hits.append((hex(x.base+rva+pos-raw),hex(pos)))
            pos+=1
    print(hex(target),hits)
