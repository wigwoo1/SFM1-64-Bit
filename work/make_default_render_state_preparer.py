from pathlib import Path
s=Path('work/prepare_derived_owner_restore_native.py').read_text().replace('0x1001fb90,0x100211e7','0x1000e550,0x1000ea5f').replace('nativeDerivedOwnerRestore','nativeDefaultRenderState').replace('derived_owner_restore_native_init.h','default_render_state_native_init.h').replace('sfm64-derived-owner-restore-native-preparation.json','sfm64-default-render-state-native-preparation.json').replace('derived_owner_restore_prepared_bytes','default_render_state_prepared_bytes')
Path('work/prepare_default_render_state_native.py').write_text(s)
