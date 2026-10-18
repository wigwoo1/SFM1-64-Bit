"""Extract fixed gamma table from original-code startup oracle records."""
from pathlib import Path
import hashlib,json,math,struct
source=Path('outputs/native-actual-power-gamma-startup.bin')
data=source.read_bytes()
argument_bytes=1536*8+5632*16
table_sizes=[256,1024*4,256*4,256*4,256*4,1024*4,4096*4,4096]
stride=argument_bytes+sum(table_sizes)
assert len(data)==108*stride
offset=argument_bytes+sum(table_sizes[:4])
table=data[offset:offset+1024]
assert all(data[case*stride+offset:case*stride+offset+1024]==table for case in range(108))
values=struct.unpack('<256f',table)
assert values[0]==0 and values[-1]==1
assert all(math.isfinite(value) and 0<=value<=1 for value in values)
assert all(a<=b for a,b in zip(values,values[1:]))
destination=Path('outputs/native-fixed-gamma22-table.bin')
destination.write_bytes(table)
report={
 'oracle_records':108,'record_bytes':stride,'table_offset_in_record':offset,
 'source_sha256':hashlib.sha256(data).hexdigest(),
 'table_sha256':hashlib.sha256(table).hexdigest(),
 'table_output':str(destination),'all_records_identical':True,
 'endpoints':[values[0],values[-1]],'midpoint_index128':values[128],
 'oracle_exe_sha256':hashlib.sha256(Path('work/sfm_actual_power_gamma_startup_native.exe').read_bytes()).hexdigest(),
 'oracle_source_sha256':hashlib.sha256(Path('work/test_actual_power_gamma_startup.cpp').read_bytes()).hexdigest(),
 'scope':'Original startup and power code executed by isolated x86 oracle with relocated data/call instrumentation; fixed table is identical across tested gamma/texture-gamma/brightness/overbright inputs.',
 'limits':'This is an oracle fixture, not a dump of the running SFM process. Runtime floating environment, selfillum parameter routing and GPU parity remain unverified.'
}
Path('outputs/native-fixed-gamma22-table-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'PASS: 108 identical native fixed tables; table[128]={values[128]:.9g}; sha256={report["table_sha256"]}')
