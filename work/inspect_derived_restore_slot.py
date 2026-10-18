from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
for table in [0x100fdc9c,0x100fd33c]:print(hex(table),[hex(im.word(table+i*4)) for i in [0,41]])
