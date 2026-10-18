"""Offline, read-only material request audit for the current unlit Vulkan path."""
import hashlib,json,re
from pathlib import Path

def parse(text):
    tokens=[]
    for match in re.finditer(r'//[^\n]*|"[^"]*"|[{}]|[^\s{}"]+',text):
        token=match.group()
        if not token.startswith('//'):tokens.append(token[1:-1] if token.startswith('"') else token)
    cursor=0
    def node():
        nonlocal cursor
        if cursor>=len(tokens):raise ValueError('Missing material value')
        value=tokens[cursor];cursor+=1
        if value!='{':return value
        entries=[]
        while cursor<len(tokens) and tokens[cursor]!='}':
            key=tokens[cursor];cursor+=1
            entries.append((key,node()))
        if cursor>=len(tokens):raise ValueError('Unterminated material block')
        cursor+=1
        return entries
    shader=tokens[cursor];cursor+=1
    values=node()
    if cursor!=len(tokens) or not isinstance(values,list):raise ValueError('Unexpected material syntax')
    return shader,values

assert parse('"VertexLitGeneric" { "$selfillum" "1" // comment\n "$mask" "models\\a" }')[1]==[('$selfillum','1'),('$mask','models\\a')]
root=Path(__file__).resolve().parent.parent
directory=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\materials\models\anno\mamagen')
shader=root/'work/sfm64-material-alpha-candidate.hlsl'
records=[]
flags={'$phong','$rimlight','$selfillum'}
paths={'$bumpmap','$envmap','$detail','$selfillummask'}
for path in sorted(directory.glob('*.vmt')):
    raw=path.read_bytes();name,values=parse(raw.decode('utf-8-sig'))
    gaps={}
    for key,value in values:
        key=key.lower()
        if isinstance(value,str) and ((key in flags and float(value)!=0) or (key in paths and value)):
            gaps.setdefault(key,[]).append(value)
    records.append(dict(material=str(path),sha256=hashlib.sha256(raw).hexdigest(),shader=name,
        unimplemented_requests_in_unlit_path=gaps,
        source_lighting_unimplemented=name.lower()=='vertexlitgeneric'))
assert records and any('$selfillummask' in record['unimplemented_requests_in_unlit_path'] for record in records)
report=dict(renderer_shader=str(shader),renderer_shader_sha256=hashlib.sha256(shader.read_bytes()).hexdigest(),
 implemented_shader_operations=['base texture sampling','alpha cutoff','per-draw alpha modulation'],
 materials=records,scope='Root-level requests in these loose VMT files compared with inspected unlit shader. Not full mount/session override/proxy resolution, native appearance comparison or renderer completion.')
output=root/'outputs/prototype-material-gap-audit.json';output.write_text(json.dumps(report,indent=2))
print(json.dumps(dict(materials=len(records),materials_with_feature_gaps=sum(bool(record['unimplemented_requests_in_unlit_path']) for record in records),report=str(output))))
