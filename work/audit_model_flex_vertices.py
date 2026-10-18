"""Read-only candidate MDL49 mesh flex layout audit. Delta encoding unresolved."""
from pathlib import Path
import hashlib,json,math,struct
path=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen.mdl')
b=path.read_bytes()
def span(p,n):
    if p<0 or n<0 or p+n>len(b):raise ValueError('MDL flex span')
def integer(p):
    span(p,4);return struct.unpack_from('<i',b,p)[0]
def table(base,count_field,offset_field,stride):
    count=integer(base+count_field);start=base+integer(base+offset_field)
    if count<0:raise ValueError('negative flex table count')
    span(start,count*stride);return range(start,start+count*stride,stride)
assert integer(0)==0x54534449 and integer(4)==49 and integer(76)==len(b)
descriptors=integer(260);meshes=[];total=0;half_nonfinite=0;types={};targets=set();samples=[]
scale=struct.unpack_from('<f',b,392)[0] if integer(152)&0x200000 else 1/4096
assert math.isfinite(scale) and scale>0
quantization_overflows=0;maximum_error=0;components=0
for body in table(0,232,236,16):
    for model in table(body,4,12,148):
        for mesh in table(model,72,76,116):
            vertex_count=integer(mesh+8)
            flexes=[]
            for flex in table(mesh,16,20,60):
                descriptor=integer(flex);pair=integer(flex+28);kind=b[flex+32]
                assert 0<=descriptor<descriptors and 0<=pair<descriptors
                assert kind in (0,1)
                types[kind]=types.get(kind,0)+1;targets.add(descriptor)
                boundaries=struct.unpack_from('<4f',b,flex+4)
                assert all(math.isfinite(v) for v in boundaries)
                records=list(table(flex,20,24,16 if kind==0 else 18))
                indices=[]
                for record in records:
                    index=struct.unpack_from('<H',b,record)[0]
                    assert index<vertex_count
                    indices.append(index)
                    half=struct.unpack_from('<6e',b,record+4)
                    half_nonfinite+=sum(not math.isfinite(v) for v in half)
                    for value in half:
                        if not math.isfinite(value):continue
                        # Float32 division then truncation, matching the SDK's
                        # conversion expression; native conversion not proven.
                        divided=struct.unpack('<f',struct.pack('<f',value/scale))[0]
                        quantized=math.trunc(divided)
                        if not -32768<=quantized<=32767:quantization_overflows+=1
                        else:
                            restored=struct.unpack('<f',struct.pack('<f',quantized*scale))[0]
                            maximum_error=max(maximum_error,abs(restored-value))
                        components+=1
                    if len(samples)<8:
                        samples.append(dict(mesh_offset=mesh,descriptor=descriptor,index=index,speed=b[record+2],side=b[record+3],raw_delta_words=list(struct.unpack_from('<6H',b,record+4)),half_interpretation=list(half)))
                total+=len(records)
                flexes.append(dict(descriptor=descriptor,pair=pair,type=kind,targets=boundaries,vertices=len(records),duplicate_indices=len(indices)-len(set(indices))))
            if flexes:meshes.append(dict(mesh_offset=mesh,vertex_count=vertex_count,flexes=flexes))
assert hashlib.sha256(path.read_bytes()).digest()==hashlib.sha256(b).digest()
report=dict(path=str(path),sha256=hashlib.sha256(b).hexdigest(),header_flags=integer(152),fixed_point_scale=scale,sdk_candidate_quantization=dict(components=components,overflows=quantization_overflows,maximum_absolute_error=maximum_error),flex_meshes=len(meshes),flexes=sum(len(m['flexes']) for m in meshes),vertex_records=total,animation_types=types,unique_descriptors=len(targets),half_interpretation_nonfinite_components=half_nonfinite,samples=samples,meshes=meshes,
    scope='Candidate SDK 32-bit disk layout audited against installed MDL49; raw 16-bit deltas retained; half/fixed-point interpretation and native morph blending unverified; no source asset writes')
Path('outputs/sfm64-model-flex-vertex-audit.json').write_text(json.dumps(report,indent=2,allow_nan=False)+'\n',encoding='utf-8')
print(json.dumps({k:report[k] for k in ('flex_meshes','flexes','vertex_records','fixed_point_scale','sdk_candidate_quantization')}))
