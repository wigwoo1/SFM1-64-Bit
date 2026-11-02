"""Shared private verification bookkeeping for the bounded border correction."""
from pathlib import Path
import hashlib,json,re
from verify_session_preview_aspect import ROOT
OUT=ROOT/'outputs/source-border-20261101'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def put(p,x):Path(p).write_text(json.dumps(x,indent=2)+'\n')
def checkpoint():
    before=json.loads((OUT/'before.json').read_text());sources=dict(before['source_sha256']);protected=dict(before['protected_sha256']);protected[before['prior']['receipt']]=before['prior']['receipt_sha256'];transitions=[]
    for t in before['transitions']:
        need(sha(t['backup'])==t['before']==sources[t['path']],'preserved explicit before source');transitions.append(dict(t,after=sha(t['path'])));sources[t['path']]=sha(t['path']);protected[t['backup']]=t['before']
    seen=set()
    def visit(p):
        p=p.resolve()
        if p in seen:return
        seen.add(p);sources[str(p)]=sha(p)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            if (p.parent/inc).exists():visit(p.parent/inc)
    for name in ['verify_source_border_common.py','verify_source_border_gpu.py','verify_source_border_draw.py','verify_source_border_editor.py','verify_source_border_reference.py']:
        p=ROOT/'work'/name
        if p.exists():visit(p)
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'source/protected identity')
    return sources,protected,transitions
