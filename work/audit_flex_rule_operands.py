"""Read-only MDL49 rule operand/name audit; no evaluator parity claim."""
from pathlib import Path
import json, struct
root=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models')
paths=[root/'anno/mamagen/mamagen.mdl']
def audit(path):
    b=path.read_bytes()
    def word(p):
        assert 0<=p<=len(b)-4
        return struct.unpack_from('<i',b,p)[0]
    def span(p,count,stride):
        assert p>=0 and count>=0 and p+count*stride<=len(b)
    def text(base):
        p=base+word(base)
        assert 0<=p<len(b)
        end=b.index(0,p)
        return b[p:end].decode('utf-8')
    assert word(0)==0x54534449 and word(4)==49 and word(76)==len(b)
    descriptors,desc_offset=word(260),word(264)
    controllers,ctrl_offset=word(268),word(272)
    count,offset=word(276),word(280)
    span(desc_offset,descriptors,4);span(ctrl_offset,controllers,20);span(offset,count,12)
    names=[text(ctrl_offset+i*20+4) for i in range(controllers)]
    desc_names=[text(desc_offset+i*4) for i in range(descriptors)]
    rules=[];invalid=[]
    for i in range(count):
        base=offset+i*12;target,ops,start=word(base),word(base+4),base+word(base+8)
        assert 0<=target<descriptors
        span(start,ops,8)
        operations=[]
        for n in range(ops):
            p=start+n*8;opcode,operand=word(p),word(p+4)
            op=dict(opcode=opcode,operand_bits=struct.unpack_from('<I',b,p+4)[0])
            if opcode==2:
                if not 0<=operand<controllers:invalid.append(dict(rule=i,index=operand))
                else:op.update(local_controller=operand,name=names[operand])
            operations.append(op)
        rules.append(dict(target=target,name=desc_names[target],operations=operations))
    by_target={}
    for r in rules:by_target.setdefault(r['target'],[]).append(r)
    repeated=[dict(target=target,name=group[0]['name'],count=len(group),
        distinct_operations=len({json.dumps(r['operations'],sort_keys=True) for r in group}))
        for target,group in by_target.items() if len(group)>1]
    return dict(path=str(path),descriptors=descriptors,controllers=controllers,
        rules=rules,invalid_controller_fetches=invalid,
        unique_rule_targets=len({r['target'] for r in rules}),
        controller_names=names,repeated_targets=repeated)
report=dict(scope='Installed read-only MDL49, candidate SDK disk layout; operand/name relationships only, no session-global weight conversion or native execution proof',models=[audit(p) for p in paths])
Path('outputs/sfm64-flex-rule-operand-audit.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
for m in report['models']:
    print(f"controllers={m['controllers']} descriptors={m['descriptors']} rules={len(m['rules'])} unique_targets={m['unique_rule_targets']} invalid_local_fetches={len(m['invalid_controller_fetches'])}")
    print(f"repeated_targets={len(m['repeated_targets'])} repeated_targets_with_different_operations={sum(r['distinct_operations']>1 for r in m['repeated_targets'])}")
    assert not m['invalid_controller_fetches']
