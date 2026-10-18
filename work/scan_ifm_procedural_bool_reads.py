from pathlib import Path
source=Path('work/scan_native_procedural_bool_reads.py').read_text().replace('work/current-datamodel-scale-disassembly.txt','work/ifm-disassembly.txt').replace('sfm64-native-procedural-bool-wide-candidates.json','sfm64-ifm-procedural-bool-candidates.json')
exec(compile(source,__file__,'exec'))
