"""Read-only flex rule coverage audit for saved multi-model scene assets."""
from pathlib import Path
from collections import Counter
import json,runpy,hashlib

audit=runpy.run_path('work/audit_flex_rule_operands.py')['audit']
assets=json.loads(Path('outputs/sfm64-multimodel-scene-assets.json').read_text(encoding='utf-8'))['models']
records=[]
seen=set()
for entry in assets:
    for candidate in entry['loose_candidates']:
        if candidate in seen:continue
        seen.add(candidate)
        path=Path(candidate)
        record=dict(path=candidate,sha256=hashlib.sha256(path.read_bytes()).hexdigest())
        try:
            data=audit(path)
            counts=Counter(op['opcode'] for rule in data['rules'] for op in rule['operations'])
            direct=all(len(r['operations'])==1 and r['operations'][0]['opcode']==2 for r in data['rules'])
            conflicting=sum(r['distinct_operations']>1 for r in data['repeated_targets'])
            record.update(controllers=data['controllers'],descriptors=data['descriptors'],rules=len(data['rules']),opcode_counts=dict(counts),direct_rule_shape=direct,conflicting_repeated_targets=conflicting,invalid_local_fetches=data['invalid_controller_fetches'])
        except (AssertionError,ValueError,IndexError,UnicodeError) as error:
            record['audit_error']=f'{type(error).__name__}: {error}'
        records.append(record)
        print(json.dumps(record))
Path('outputs/sfm64-scene-flex-rule-coverage.json').write_text(json.dumps(dict(models=records,scope='Installed read-only assets; disk rule shape only, no native evaluator or render parity proof'),indent=2)+'\n',encoding='utf-8')
