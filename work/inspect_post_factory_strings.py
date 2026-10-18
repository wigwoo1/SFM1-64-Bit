from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
for address in [0x100fd784,0x100fd774,0x100fda68]:print(hex(address),repr(im.string(address)))
