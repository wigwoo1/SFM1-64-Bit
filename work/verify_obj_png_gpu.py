from pathlib import Path
import runpy,subprocess,struct,zlib,json
state=runpy.run_path('work/verify_obj_gpu_import.py');out=state['out'];obj=state['obj']
def png(path,rgb):
 def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
 path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',1,1,8,6 if len(rgb)==4 else 2,0,0,0))+chunk(b'IDAT',zlib.compress(b'\0'+bytes(rgb)))+chunk(b'IEND',b''))
red=out/'red.png';blue=out/'blue.png';png(red,(255,0,0));png(blue,(0,0,255));folder=out/'textured';folder.mkdir(exist_ok=True)
env=state['env'].copy();env['SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY']=str(folder.resolve());args=state['args']+['--obj-texture','left',str(red),'--obj-texture','right',str(blue)]
run=subprocess.run(args,env=env,capture_output=True,text=True,timeout=60);log=run.stdout+run.stderr;(out/'textured.txt').write_text(log);assert run.returncode==0,log
pixels=(folder/'vulkan-retained-frame-0.rgba').read_bytes();redCount=sum(pixels[i:i+3]==b'\xff\0\0' for i in range(0,len(pixels),4));blueCount=sum(pixels[i:i+3]==b'\0\0\xff' for i in range(0,len(pixels),4));assert redCount>10000 and blueCount>10000,(redCount,blueCount)
invalid=subprocess.run(state['args']+['--obj-texture','missing',str(red)],env=env,capture_output=True,text=True,timeout=30);assert invalid.returncode!=0 and 'OBJ texture material does not exist' in invalid.stderr,(invalid.stdout,invalid.stderr)
transparent=out/'transparent.png';png(transparent,(255,0,0,64));broken=out/'broken.png';broken.write_bytes(b'not a PNG')
for path,message in [(transparent,'OBJ transparent PNG rendering is not implemented'),(broken,'PNG decode failed')]:
 check=subprocess.run(state['args']+['--obj-texture','left',str(path)],env=env,capture_output=True,text=True,timeout=30);assert check.returncode!=0 and message in check.stderr,(check.stdout,check.stderr)
report=dict(transparent_png_rejected=True,corrupt_png_rejected=True,actual_gpu=True,red_pixels=redCount,blue_pixels=blueCount,unknown_material_rejected=True,png_format='RGB decoded to RGBA',limits='Opaque PNG assignments via launch arguments; transparent PNGs, texture UI and persistence unfinished. Solid-color fixtures do not verify UV orientation.')
(out/'png-report.json').write_text(json.dumps(report,indent=2)+'\n');print('obj_png_material_gpu_assignments=passed red_pixels='+str(redCount)+' blue_pixels='+str(blueCount)+' unknown_material_rejection=passed')

