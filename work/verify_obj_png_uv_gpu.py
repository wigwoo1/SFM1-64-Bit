from pathlib import Path
import runpy,subprocess,struct,zlib,json,math
state=runpy.run_path('work/verify_obj_gpu_import.py');out=state['out'];original=state['obj'].read_text();positions=[line for line in original.splitlines() if line.startswith('v ')];obj=out/'uv-quad.obj';obj.write_text('\n'.join(positions)+'\nvt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nusemtl left\nf 1/1 2/2 3/3\nusemtl right\nf 1/1 3/3 4/4\n')
def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
# PNG rows: red/green, blue/yellow; no filter, RGB8.
texture=out/'quadrants.png';texture.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',2,2,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(bytes([0,255,0,0,0,255,0,0,0,0,255,255,255,0])))+chunk(b'IEND',b''))
folder=out/'uv-textured';folder.mkdir(exist_ok=True);env=state['env'].copy();env['SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY']=str(folder.resolve());args=state['args'].copy();args[args.index('--obj')+1]=str(obj);args+=['--obj-texture','left',str(texture),'--obj-texture','right',str(texture)]
run=subprocess.run(args,env=env,capture_output=True,text=True,timeout=60);log=run.stdout+run.stderr;(out/'uv-textured.txt').write_text(log);assert run.returncode==0,log
pixels=(folder/'vulkan-retained-frame-0.rgba').read_bytes()
probes={name:list(pixels[(y*512+x)*4:(y*512+x)*4+3]) for name,x,y in [('upper_left',160,160),('upper_right',350,160),('lower_left',160,350),('lower_right',350,350)]}
colors=[[(255,0,0),(0,255,0)],[(0,0,255),(255,255,0)]]
def expected(x,y):
 # Fixture projection covers NDC[-.6,.6], positive-height 512 viewport.
 u=(x+.5-102.4)/307.2;v=(y+.5-102.4)/307.2
 tx=u*2-.5;ty=v*2-.5;ix=math.floor(tx);iy=math.floor(ty);fx=tx-ix;fy=ty-iy
 return [sum(colors[(iy+dy)%2][(ix+dx)%2][c]*(fx if dx else 1-fx)*(fy if dy else 1-fy) for dy in [0,1] for dx in [0,1]) for c in range(3)]
maximum=0;checked=0
for y in range(135,390,19):
 for x in range(135,390,19):
  actual=pixels[(y*512+x)*4:(y*512+x)*4+3];target=expected(x,y);error=max(abs(actual[c]-target[c]) for c in range(3));maximum=max(maximum,error);assert error<=2,(x,y,list(actual),target,error);checked+=1
report=dict(actual_gpu=True,checked_pixels=checked,maximum_channel_error=maximum,filter='bilinear repeat',orientation='PNG first decoded row corresponds to GPU v=0; OBJ UV values retained',probes=probes,limits='One planar fixture with RGB8 opaque PNG. UV adjustment UI, other image formats, transparency and persistence unfinished.')
(out/'uv-report.json').write_text(json.dumps(report,indent=2)+'\n')
print('obj_nonuniform_png_gpu_uv_sampling=passed checked_pixels='+str(checked)+' maximum_channel_error='+str(maximum))
(out/'uv-probes.json').write_text(json.dumps(probes,indent=2)+'\n')

