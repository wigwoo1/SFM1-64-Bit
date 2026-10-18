from pathlib import Path
s=Path('work/prepare_owner_post_reset_native.py').read_text().replace('0x1002b4c0,0x1002b653','0x1001fb90,0x100211e7').replace('nativeOwnerPostReset','nativeDerivedOwnerRestore').replace('owner_post_reset_native_init.h','derived_owner_restore_native_init.h').replace('sfm64-owner-post-reset-native-preparation.json','sfm64-derived-owner-restore-native-preparation.json').replace('owner_post_reset_prepared_bytes','derived_owner_restore_prepared_bytes').replace('assert len(calls)==7','assert calls')
Path('work/prepare_derived_owner_restore_native.py').write_text(s)
