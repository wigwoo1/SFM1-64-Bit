import json,hashlib
from pathlib import Path
paths=[Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll'),Path('work/time-to-frame-original-code.bin'),Path('outputs/sfm-time-to-frame-original-oracle.bin')]
r=dict(cases=1600040,time_range=[-20000,20000],rates=[[24,10000],[30,10010],[60,10010],[1,1],[65535,1],[0,10000],[7,9999],[65535,65535]],modes=[0,1,2,3,99],files=[dict(path=str(p),sha256=hashlib.sha256(p.read_bytes()).hexdigest(),bytes=p.stat().st_size) for p in paths],result='Isolated original x86 routine with original multiply/divide helpers exactly matched x64 candidate; input time remained unchanged.',limitations='Finite sampled rates and bounded time range, nonzero denominator only. Does not establish editor runtime ABI or whole adjustment path parity.')
Path('outputs/sfm-time-to-frame-original-parity.json').write_text(json.dumps(r,indent=2)+'\n')
