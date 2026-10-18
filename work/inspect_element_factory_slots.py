from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
print([(i,hex(im.word(0x10b3737c+4*i))) for i in range(8)])
