"""Offline complete original vertex writers versus owned x64 weight handoff.

Only their bone-bearing, optional-tangent-absent branch is executed. Original
code is privately mapped without loader/import initialization or code patches.
"""
from collections import Counter
from datetime import datetime,timezone
import hashlib
import itertools
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from verify_ifm_draw_packet import check_ranges
from verify_studio_skin_palette import STUDIO,STUDIO_SHA

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/studio-vertex-weights-20261025'

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def require(value,message):
    if not value:raise RuntimeError(message)


class Reader:
    def __init__(self,b):self.b=b
    def span(self,a,n):
        require(a>=0 and n>=0 and a<=len(self.b) and n<=len(self.b)-a,'asset span')
        return self.b[a:a+n]
    def u(self,a):return struct.unpack('<I',self.span(a,4))[0]
    def h(self,a):return struct.unpack('<H',self.span(a,2))[0]
    def table(self,base,count,relative,stride):
        n=self.u(base+count);require(n<=1000000,'asset count limit')
        start=base+struct.unpack('<i',self.span(base+relative,4))[0]
        self.span(start,n*stride);return [start+i*stride for i in range(n)]


def benchmark_vertices(mdl,vvd,vtx):
    m,v,t=map(Reader,[mdl,vvd,vtx]);require(m.u(0)==0x54534449 and m.u(4)==49 and m.u(76)==len(mdl),'MDL identity')
    checksum=m.u(8);require(v.u(0)==0x56534449 and v.u(4)==4 and v.u(8)==checksum and t.u(0)==7 and t.u(16)==checksum,'VVD/VTX identity')
    count=v.u(16);require(count<1000000,'VVD count');ids=[]
    if v.u(48):
        for i in range(v.u(48)):
            at=v.u(52)+i*12;level,source,n=struct.unpack('<3I',v.span(at,12));require(level<v.u(12) and source+n<=count,'VVD fixups')
            ids.extend(range(source,source+n))
    else:ids=list(range(count))
    require(len(ids)==count,'VVD complete LOD0 fixups')
    vertices=[v.span(v.u(56)+i*48,48) for i in ids];metadata={}
    for bi,body in enumerate(m.table(0,232,236,16)):
        for mi,model in enumerate(m.table(body,4,12,148)):
            require(m.u(model+84)%48==0,'model vertex alignment');first=m.u(model+84)//48
            for mei,mesh in enumerate(m.table(model,72,76,116)):
                require(m.u(mesh+12)+m.u(mesh+8)<=m.u(model+80),'model mesh span')
                metadata[bi,mi,mei]=(first+m.u(mesh+12),m.u(mesh+8))
    result=[];groups=0;strips=0;changes_count=0;index_refs=0;bone_refs=0;reassigned=0
    for bi,body in enumerate(t.table(0,28,32,8)):
        for mi,model in enumerate(t.table(body,0,4,8)):
            lods=t.table(model,0,4,12);require(lods,'VTX LODs')
            for mei,mesh in enumerate(t.table(lods[0],0,4,9)):
                require((bi,mi,mei) in metadata,'VTX mesh metadata');first,n=metadata.pop((bi,mi,mei))
                for group in t.table(mesh,0,4,33):
                    groups+=1
                    group_vertices=t.table(group,0,4,9)
                    for at in group_vertices:
                        original=t.h(at+4);require(original<n and first+original<len(vertices),'VTX original mesh vertex')
                        result.append((vertices[first+original],t.span(at,9)))
                    # Dataset consistency proof, not execution of the original
                    # renderer's strip submission/material-context callbacks.
                    indices=t.table(group,8,12,2);mapping={}
                    for strip in t.table(group,16,20,35):
                        strips+=1;begin=t.u(strip+4);length=t.u(strip)
                        require(begin+length<=len(indices) and length%3==0 and t.span(strip+18,1)==b'\x01','VTX strip triangle range')
                        vertex_first=t.u(strip+12);vertex_count=t.u(strip+8)
                        require(vertex_first+vertex_count<=len(group_vertices),'VTX strip vertex span')
                        for at in t.table(strip,19,23,8):
                            slot,bone=struct.unpack('<2i',t.span(at,8));require(0<=slot<53 and 0<=bone<m.u(156),'VTX bone state span')
                            reassigned+=slot in mapping and mapping[slot]!=bone;mapping[slot]=bone;changes_count+=1
                        for at in indices[begin:begin+length]:
                            vi=t.h(at);require(vertex_first<=vi<vertex_first+vertex_count,'VTX referenced strip vertex')
                            optimized=t.span(group_vertices[vi],9);source=vertices[first+struct.unpack_from('<H',optimized,4)[0]];index_refs+=1
                            require(optimized[3]<=t.h(strip+16),'VTX strip influence maximum')
                            for influence in range(optimized[3]):
                                wi=optimized[influence];slot=optimized[6+influence]
                                require(wi<source[15] and slot in mapping and mapping[slot]==source[12+wi],'Benchmark strip hardware slot disagrees with VVD model bone')
                                bone_refs+=1
    require(not metadata,'Missing VTX meshes');return result,dict(vvd_vertices=count,optimized_vertices=len(result),groups=groups,
        strips=strips,bone_state_changes=changes_count,reassigned_hardware_slots=reassigned,strip_index_references=index_refs,strip_bone_reference_checks=bone_refs)


def fixture(vertex,opt,compressed,stored):
    require(len(vertex)==48 and len(opt)==9,'query record size')
    return vertex+opt+bytes([compressed,stored,0])+bytes.fromhex('1234567812345678')


def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    original={str(STUDIO):sha(STUDIO)};require(original[str(STUDIO)]==STUDIO_SHA,'Installed studiorender pin')
    image,evidence=check_ranges(STUDIO,'studiorender-disassembly.txt',[
        ('complete_compressed_vertex_writer',0x10055aa0,0x100560be),
        ('complete_uncompressed_vertex_writer',0x100560c0,0x10056518),
        ('complete_packed_normal_helper',0x1000a100,0x1000a210),
        ('complete_vertex_builder_advance',0x1000a210,0x1000a28d),
        ('cookie_success_path',0x1005bdd1,0x1005bddb),
        ('caller_selects_two_vertex_writers',0x100567f0,0x10056845)])
    require(image.word(0x100711f4)==0x47000000,'32768 packing scale')
    require(struct.unpack('<d',image.b[image.offset(0x100702b0):image.offset(0x100702b0)+8])[0]==0.001,'sum tolerance')
    tracked=['studio_vertex_weights_candidate.h','probe_studio_vertex_weights.cpp','test_studio_vertex_weights.cpp','build_studio_vertex_weights.cmd','verify_studio_vertex_weights.py']
    sources={p:sha(ROOT/'work'/p) for p in tracked}
    binaries=[]
    for tag,name,machine in [('native','vertex_weights_native_x86',0x14c),('owned','vertex_weights_owned_x64',0x8664),('safety','vertex_weights_safety_x64',0x8664)]:
        log=(OUT/(tag+'-build.log')).read_text();require('warning ' not in log.lower() and 'error ' not in log.lower(),'Build diagnostics '+tag)
        p=OUT/(name+'.exe');b=p.read_bytes();pe=struct.unpack_from('<I',b,60)[0]
        require(b[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==(0x10b if machine==0x14c else 0x20b),'Binary architecture')
        binaries.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=120)
        (run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,name+' failed '+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout
    safety=execute('vertex_weights_safety_x64',[],'safety');require('checks=6 refusals=11' in safety,'Safety count')
    rows=[];rng=random.Random(2026102508)
    # Exact and near-threshold sums, zeros, non-normalized and quantization edges.
    weights=[(1,0,0),(0,1,0),(0,0,1),(0,0,0),(0.25,0.5,0.25),(1/3,1/3,1/3),(0.2,0.2,0.2),
        (0.999,0,0),(0.99901,0,0),(0.99899,0,0),(0.50049,0.5,0),(0.50051,0.5,0),
        (1/32768,1-1/32768,0),(0.5-1/65536,0.5+1/65536,0),(-0.0,1,0)]
    for w,n,order,indices in itertools.product(weights,range(4),[(0,1,2),(2,0,1),(1,2,0),(0,0,0)],[(0,1,52),(255,52,0)]):
        vertex=bytearray(48);struct.pack_into('<3f',vertex,0,*w);vertex[15]=3;struct.pack_into('<3f',vertex,28,0,0,1)
        opt=bytes([*order,n,0,0,*indices]);rows.extend(fixture(vertex,opt,c,k) for c,k in [(0,2),(1,1),(1,2)])
    for _ in range(2048):
        w=[rng.random() for _ in range(3)];total=sum(w);w=[x/total for x in w]
        vertex=bytearray(48);struct.pack_into('<3f',vertex,0,*w);vertex[15]=rng.randrange(1,4);struct.pack_into('<3f',vertex,28,0,0,1)
        order=list(range(3));rng.shuffle(order);opt=bytes([*order,rng.randrange(4),0,0,*[rng.choice([0,1,26,52,255]) for _ in range(3)]])
        rows.extend(fixture(vertex,opt,c,k) for c,k in [(0,2),(1,1),(1,2)])
    synthetic_count=len(rows);benchmark_count=0;models=[]
    previous=json.loads((ROOT/'outputs/studio-skin-palette-20261025/verification-sn3w4epv/results.json').read_text())
    assets={str(Path(a['path'])):a['sha256'] for a in previous['assets']}
    for base in [Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/usermod/models/player/hwm/pyro'),Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/usermod/models/weapons/w_models/w_flamethrower')]:
        data=[]
        for ext in ['.mdl','.vvd','.dx90.vtx']:
            p=Path(str(base)+ext);original[str(p)]=sha(p);require(original[str(p)]==assets[str(p)],'Benchmark asset pin');data.append(p.read_bytes())
        pairs,stats=benchmark_vertices(*data);counts=Counter();reordered=0;invalid255=0
        for vertex,opt in pairs:
            counts[opt[3]]+=1;reordered+=any(opt[i]!=i for i in range(opt[3]));invalid255+=any(opt[6+i]==255 for i in range(opt[3]))
            for c,k in [(0,2),(1,1),(1,2)]:rows.append(fixture(vertex,opt,c,k));benchmark_count+=1
        models.append(dict(model=str(base),**stats,optimized_influence_counts=dict(counts),reordered_vertices=reordered,index255_vertices=invalid255))
    query=run/'queries.bin';query.write_bytes(b''.join(rows));outputs=[]
    for tag in ['native_x86','owned_x64']:
        p=run/(tag+'.bin');execute('vertex_weights_'+tag,[STUDIO,query,p],tag);outputs.append(p.read_bytes())
    require(len(outputs[0])==len(rows)*16,'Native output size')
    if outputs[0]!=outputs[1]:
        different=next(i for i in range(len(rows)) if outputs[0][i*16:i*16+16]!=outputs[1][i*16:i*16+16]);raise RuntimeError('Native/owned differ at case '+str(different)+' query='+rows[different].hex()+' native='+outputs[0][different*16:different*16+16].hex()+' owned='+outputs[1][different*16:different*16+16].hex())
    invalid=sum(not outputs[0][i*16+12] for i in range(len(rows)));benchmark_invalid=sum(not outputs[0][i*16+12] for i in range(synthetic_count,len(rows)))
    for p,digest in original.items():require(sha(p)==digest,'Original changed '+p)
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(rows),synthetic_cases=synthetic_count,benchmark_cases=benchmark_count,
        exact_output_bytes=len(outputs[0]),synthetic_and_benchmark_invalid_sum_flags=invalid,benchmark_invalid_sum_flags=benchmark_invalid,
        models=models,safety=safety.strip(),executables=binaries,sources_sha256=sources,original_sha256=original,
        static_ranges=evidence,query_sha256=sha(query),output_sha256=hashlib.sha256(outputs[0]).hexdigest(),
        computer_use=False,sfm_process_used=False,original_unchanged=True,editor_changed=False,
        limitations='Full original writer code unpatched, optional tangent absent, bone-bearing VVD1..3/VTX0..3 branch. Compare weights, hardware-index bytes and sum-validity only; normal/position/UV/color and builder advancement execute but are not parity targets. Benchmark strip mapping has per-reference dataset consistency checks, not execution of native strip callbacks. Actual live compression/declaration choice, general native strip submission, shader unpacking, GPU skinning and viewport integration unverified.')
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(status='PASS',receipt=str(run/'results.json'),cases=len(rows),benchmark_cases=benchmark_count,exact_output_bytes=len(outputs[0]),benchmark_invalid_sum_flags=benchmark_invalid,models=models)))


if __name__=='__main__':main()
