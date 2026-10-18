from pathlib import Path
source=Path('work/prepare_query_cleanup_native.py').read_text()
source=source.replace('0x1002b300,0x1002b3c0','0x1002b3c0,0x1002b4c0').replace('assert not calls','assert len(calls)==5').replace('nativeQueryCleanup','nativeOwnerResetCleanup').replace('query_cleanup_native_init.h','owner_reset_cleanup_native_init.h').replace('sfm64-query-cleanup-native-preparation.json','sfm64-owner-reset-cleanup-native-preparation.json').replace('query_cleanup_prepared_bytes','owner_reset_cleanup_prepared_bytes')
source=source.replace("for name,entries in [('Fixups',fixups)]", "for name,entries in [('Fixups',fixups),('Calls',calls)]")
source=source.replace("Path('work/owner_reset_cleanup_native_init.h').write_text(header)", """branches=[]
starts={int(row['address'],16) for row in report['instructions'] if start<=int(row['address'],16)<end}
for row in report['instructions']:
 address=int(row['address'],16)
 if start<=address<end and row['instruction'].startswith('j'):
  target=int(row['instruction'].split()[1],16)
  assert start<=target<end and target in starts
  branches.append({'address':hex(address),'target':hex(target)})
assert len(branches)==5
Path('work/owner_reset_cleanup_native_init.h').write_text(header)""")
source=source.replace("'native_executed':False", "'branches':branches,'native_executed':False")
Path('work/prepare_owner_reset_cleanup_native.py').write_text(source)
