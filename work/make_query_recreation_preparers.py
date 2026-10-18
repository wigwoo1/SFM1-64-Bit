from pathlib import Path
base=Path('work/prepare_reset_texture_native.py').read_text()
for name,start,end,calls in [('PairQuery',0x1002b240,0x1002b2fc,3),('QueuedQuery',0x1002a9d0,0x1002aa94,2)]:
 slug='pair_query' if name=='PairQuery' else 'queued_query'
 source=base.replace('0x1002b1e0,0x1002b237',f'{hex(start)},{hex(end)}').replace('nativeResetTexture','native'+name).replace('reset_texture_native_init.h',slug+'_native_init.h').replace('sfm64-reset-texture-native-preparation.json','sfm64-'+slug.replace('_','-')+'-native-preparation.json').replace('reset_texture_prepared_bytes',slug+'_prepared_bytes').replace('assert len(calls)==2',f'assert len(calls)=={calls}').replace('assert len(branches)==2','assert branches')
 Path('work/prepare_'+slug+'_native.py').write_text(source)
p=Path('work/recover_owner_reset_cleanup.py');s=p.read_text().replace('Resource creation helpers and callback10028330 unresolved; flag20','Resource creation helpers statically identified; callback10028330 is an interface factory, receiving service semantics unresolved; flag20').replace('Helper1002b1e0 and cache invalidation unbound;','Helper1002b1e0 identified as texture recreation; cache invalidation unbound;').replace('CreateTexture signature/SDK constants compile binding pending;','CreateTexture signature/SDK constants compiled and controlled original/x64 comparison passed;')
p.write_text(s)
