"""Verify intact server standard rules and original model getter vs owned x64."""
import argparse
import ast
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
from verify_server_simple_trace_filter import PINS, SERVER, sha
from verify_sliding_door_spawn import verify_installed_ranges

ROOT=Path(__file__).resolve().parent
def closure():
    result={}
    def visit(path):
        path=path.resolve();key=path.relative_to(ROOT).as_posix()
        if key in result:return
        result[key]=sha(path);text=path.read_text(encoding='utf-8-sig')
        for name in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',text,re.M):visit(path.parent/name)
        if path.suffix=='.py':
            for node in ast.walk(ast.parse(text)):
                names=[a.name for a in node.names] if isinstance(node,ast.Import) else [node.module] if isinstance(node,ast.ImportFrom) and node.module else []
                for name in names:
                    local=ROOT/(name.split('.')[0]+'.py')
                    if local.is_file():visit(local)
    for name in ('probe_server_standard_trace_rules.cpp','build_server_standard_trace_rules.cmd','verify_server_standard_trace_rules.py'):visit(ROOT/name)
    return result

def prediction():
    records=[];counts=dict(static_early=0,missing_base_early=0,monster_reject=0,model_flags_reject=0,movement_reject=0,accepted=0,both_model_queries=0,first_false_second_still_called=0)
    for flags in range(4):
        for model in (-1,0,1,2):
            for solid in (-1,0,1,6,7):
                for move in (0,6,7,8,255):
                    for first in (0,1,0x100,0x101):
                        for second in (0,1,0x100,0x101):
                            for selector in range(10):
                                mask=((2 if selector&1 else 0)|(0x4000 if selector&2 else 0)|(0x2000000 if selector&4 else 0)) if selector<8 else (0x200400b if selector==8 else 0xffffffff)
                                events=[1];answer=True
                                if flags&1:counts['static_early']+=1
                                else:
                                    events.append(2)
                                    if not flags&2:counts['missing_base_early']+=1
                                    else:
                                        events.extend((3,4,5,6))
                                        monster_ok=(model==1 and solid in (1,6)) or bool(mask&0x2000000)
                                        if not monster_ok:
                                            answer=False;counts['monster_reject']+=1
                                        else:
                                            if not mask&2:
                                                events.extend((7,8));counts['both_model_queries']+=1
                                                counts['first_false_second_still_called']+=(first&255)==0
                                            window_ok=bool(mask&2) or not(first&255) or bool(second&255)
                                            movement_ok=bool(mask&0x4000) or move!=7
                                            answer=window_ok and movement_ok
                                            counts['model_flags_reject']+=not window_ok
                                            counts['movement_reject']+=window_ok and not movement_ok
                                            counts['accepted']+=answer
                                records.append(struct.pack('<IIii14I',flags,mask,model,solid,move,first,second,int(answer),len(events),*(events+[0]*(9-len(events)))))
    assert len(records)==64000 and all(counts.values())
    return b''.join(records),counts

def main(directory):
    directory=directory.resolve();assert directory.is_relative_to(Path('D:/SFM64-development-tests').resolve())
    directory.mkdir(parents=True,exist_ok=True);assert all(sha(p)==v for p,v in PINS.items())
    before=closure();run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    build=subprocess.run(['cmd','/c',str(ROOT/'build_server_standard_trace_rules.cmd'),str(run)],capture_output=True,text=True,timeout=180)
    (run/'build.log').write_text(build.stdout+build.stderr);assert build.returncode==0,build.stdout+build.stderr
    _,spans=verify_installed_ranges(SERVER,run/'reference.local.txt',[
        ('complete server standard rules',0x102676b0,0x1026776d),
        ('complete original model getter',0x100b4aa0,0x100b4abd)])
    expected,counts=prediction();builds={};runs=[]
    for arch,machine,magic in (('x86',0x14c,0x10b),('x64',0x8664,0x20b)):
        log=(run/f'{arch}-build.log').read_text(errors='replace');assert not re.search(r'\bwarning\b|\berror\b',log,re.I),log
        exe=run/f'standard_rules_{arch}.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        assert raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
        for repeat in (1,2):
            output=run/f'{arch}-{repeat}.bin';result=subprocess.run([str(exe),str(SERVER) if arch=='x86' else '-',str(output)],capture_output=True,text=True,timeout=120)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode==0,result.stdout+result.stderr
            assert json.loads(result.stdout)==dict(cases=64000,passed=True)
            assert output.read_bytes()==expected,('native/owned/independent decision or events differ',arch,repeat)
            runs.append(dict(architecture=arch,repeat=repeat,record_sha256=sha(output)))
    # Accepted caller source and executable guards are checked without modifying
    # its old receipt or changing its immutable record digest.
    previous=Path('D:/SFM64-development-tests/20261128-server-simple-filter/verify-oub0xlpo/results.json')
    accepted=json.loads(previous.read_text());assert accepted['record_sha256']=='a4457f34efb8e591e88f81a94fd2faafd94cdd55504dec82df0c7cefe26b530d'
    assert all(sha(ROOT/name)==value for name,value in accepted['source_sha256'].items())
    assert all(sha(v['executable'])==v['sha256'] for v in accepted['builds'].values())
    assert closure()==before and all(sha(p)==v for p,v in PINS.items())
    editor=json.loads((ROOT.parent/'publication/stage/docs/build-status.json').read_text())
    deps=editor['editor_source_dependencies'];assert len(deps)==543 and all(sha(ROOT.parent/p)==v for p,v in deps.items())
    receipt=dict(status='VERIFIED BOUNDED SERVER STANDARD TRACE RULES',cases_per_run=64000,repeats_per_arch=2,
        builds=builds,runs=runs,coverage=counts,source_sha256=before,record_bytes=len(expected),record_sha256=hashlib.sha256(expected).hexdigest(),
        verified_spans=spans,original_sha256={str(p):v for p,v in PINS.items()},originals_unchanged=True,editor_dependencies_unchanged=543,
        accepted_caller_receipt=str(previous),accepted_caller_sources_and_binaries_unchanged=True,
        findings=['Complete original standard-rules predicate and original model getter execute unchanged; no hardware boundary substitutes any static callee.',
                  'All model/solid types, static/missing-base early returns, mask gates, movement-byte7 and two distinct model virtuals match independent decision/event records.',
                  'Both model virtuals execute when bit2 is absent, even when the first returns false; only AL is tested. Raw return values0/1/0x100/0x101 distinguish this.',
                  'Original model getter obtains entity model index via virtual+18 and model via model-info virtual+4; supplied bindings verify owner and negative index argument.',
                  'Owned identity fixtures exceed32bits; no native addresses enter production helper state.'],
        supplied_services=['static manager classification','handle base resolution','solid virtual+2c','entity model index virtual+18',
                           'model-info lookup/type/flags virtuals+4/+24/+30/+34'],
        limits=['Model virtual implementations and their exact material meanings remain host dependencies; two flag queries are kept distinct rather than guessed into one transparency flag.',
                'Actual entity/model loading, static handle classification, ignored-owner relationships, game collision rules, complete engine hull composition, spawn and editor integration remain separate gates.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'],coverage=counts,receipt=str(run/'results.json'))))
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);main(parser.parse_args().directory)
