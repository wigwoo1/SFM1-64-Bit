from pathlib import Path
import json,struct
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
body=region(0x100d5020,0x100d5368)
boundaries={int(r['address'],16) for r in body['instructions']}
for row in body['branches']:
 a=int(row['address'],16);b=bytes.fromhex(row['bytes'])
 if b[0]==0x0f: target=a+6+struct.unpack_from('<i',b,2)[0]
 else: target=a+2+struct.unpack_from('<b',b,1)[0]
 assert target in boundaries
assert [int(c['target'],16) for c in body['direct_calls']]==[0x100e1154]*3+[0x100e01aa]*4
spec=[('gammaLimit',0x100fc1f4,'f'),('oneDouble',0x100fc7f0,'d'),('oneFloat',0x100fa5f8,'f'),('thresholdLow',0x10100198,'f'),('thresholdHigh',0x100fc20c,'f'),('thresholdBase',0x10141398,'d'),('brightnessSquareScale',0x10152458,'d'),('byteScale',0x100fc760,'f'),('inverse255',0x100fc764,'f'),('inverse1023',0x10152450,'d'),('upperSlope',0x10152448,'d'),('fixedExponent0454',0x10152400,'d'),('fixedExponent22',0x100fc898,'d'),('inverseScale',0x100fffb0,'d'),('overbright2',0x100fc88c,'f'),('overbright4',0x10100194,'f'),('inverse1024',0x10152440,'d')]
constants=[];header='#pragma once\nstruct StudioGammaConstants {\n'
for name,address,fmt in spec:
 size=struct.calcsize(fmt);raw=im.b[im.offset(address):im.offset(address)+size];value=struct.unpack('<'+fmt,raw)[0]
 constants.append(dict(name=name,address=hex(address),file_offset=im.offset(address),bytes=raw.hex(),value=value))
 header+=('float' if fmt=='f' else 'double')+' '+name+';\n'
header+='};\nstatic constexpr StudioGammaConstants studioGammaConstants={'+','.join(format(c['value'],'.17g')+('f' if spec[i][2]=='f' and '.' in format(c['value'],'.17g') else '.0f' if spec[i][2]=='f' else '') for i,c in enumerate(constants))+'};\n'
Path('work/studio_gamma_constants.h').write_text(header)
tables=[('textureGamma',0x101985d0,256,1),('linearToScreenGamma',0x101931d0,1024,4),('textureToLinear',0x101909d0,256,4),('fixedPower0454',0x10191dd0,256,4),('fixedPower22',0x101941d0,256,4),('linearToTexture',0x101921d0,1024,4),('linearToVertex',0x101945d0,4096,4),('linearToLightmap',0x10190dd0,4096,1)]
report=dict(sha256=sha,body=body,constants=constants,tables=[dict(name=n,address=hex(a),entries=e,element_bytes=w) for n,a,e,w in tables],finding='Eight output tables. Gamma clamp affects first screen/texture stages; final vertex/lightmap exponent reloads original gamma. Brightness stack slot is reused after screen table. Final lightmap conversion uses x87 FISTP rounding, unlike earlier truncating SSE conversions.',limits='Structural recovery only; optimized float/double pow providers and runtime rounding parity not proved.')
Path('outputs/sfm64-gamma-startup-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_gamma_builder_bytes=840 tables=8 constants=17 direct_pow_calls=7')

