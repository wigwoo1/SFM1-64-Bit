from pathlib import Path
p=Path('work/recover_controller_title_imports.py').read_text().replace('targets={0x10a7cc04,0x10a7cc00,0x10a7b8fc,0x10a7b940,0x10a7b998,0x10a7b99c}','targets={0x10a7cc80,0x10a7b414}').replace('controller-title-import-bindings','controller-cleanup-import-bindings')
Path('work/recover_controller_cleanup_imports.py').write_text(p)
