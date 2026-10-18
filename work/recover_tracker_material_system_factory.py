from pathlib import Path
import json,hashlib,struct,re
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll');sha=hashlib.sha256(im.b).hexdigest();rows=[];pending=None
for line in Path('work/materialsystem-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if m:pending=dict(address=hex(int(m[1],16)),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
def region(start,end):
 selected=[x for x in rows if start<=int(x['address'],16)<end];cursor=start
 for x in selected:
  a=int(x['address'],16);b=bytes.fromhex(x['bytes']);assert a==cursor and im.b[im.offset(a):im.offset(a)+len(b)]==b;cursor+=len(b)
 assert cursor==end
 return dict(start=hex(start),end=hex(end),bytes=end-start,instructions=selected)
connect=region(0x1000f740,0x1000f839);wrapper=region(0x1000c2c0,0x1000c35c);getter=region(0x10070260,0x10070266)
labels={hex(a):im.string(a) for a in [0x1009700c,0x10096fec,0x10096fdc,0x10096fcc,0x10096fbc,0x10096888,0x10096870,0x10096860]}
assert labels['0x1009700c']=='ShaderDeviceMgr001'
assert im.word(0x1000f82b)==0x1000c2c0 and im.word(0x10070261)==0x1006fd90
opt=im.u32(60)+24;directory=im.base+im.u32(opt+96);exports=[]
if directory!=im.base:
 for i in range(im.word(directory+24)):
  name=im.string(im.base+im.word(im.base+im.word(directory+32)+4*i));ordinal=im.u16(im.offset(im.base+im.word(directory+36)+2*i));address=im.base+im.word(im.base+im.word(directory+28)+4*ordinal)
  if address==0x10108960:exports.append(name)
report=dict(sha256=sha,connect=connect,wrapper=wrapper,local_factory_getter=getter,labels=labels,upstream_factory_cell='0x10108960',matching_cell_exports=exports,
finding='MaterialsystemConnect1000f740 usesreceiver+29d8 shaderfactory torequestShaderDeviceMgr001 andfourotherinterfaces. Storesmgr1011b09c andcallsitsvtable slot0 withmaterialwrapper1000c2c0. Wrapper directlyhandles three namedinterfaces; othernames call optionalfactory10108960(name,nullstatus), then localregistry1006fd90 via10070260. Optionalwrapperstatus initialized0 andset1 onlywhenbothfallbacksnull. Therefore shadertrackerwrapper externalfirst reachesmaterialwrapper upstream/localregistry before shaderlocalfallback.',
limits='Fullselected materialmodule bytes verified. Three-name comparison semantics andlocalregistry fullbody notverifiedhere; upstreamfactorycellruntimevalue/writes/selection remainunresolved. No fullmaterialConnect execution orGPUstartupclaim.',
next_action='Recover materiallocalregistry andupstreamfactorycell binding before composingnestedfactorywrappers andchoosingtrackerprovider.')
Path('outputs/sfm64-tracker-material-system-factory.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_material_tracker_factory_bytes='+str(connect['bytes']+wrapper['bytes']+getter['bytes'])+' upstream_cell_exports='+str(exports))
