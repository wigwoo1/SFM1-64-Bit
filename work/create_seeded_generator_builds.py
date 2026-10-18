from pathlib import Path
for kind,arch in [('native','32'),('compare','64')]:
 s=Path('work/build_gamma_startup_'+kind+'.cmd').read_text().replace('test_gamma_startup_compare.cpp','test_seeded_generator.cpp').replace('gamma_startup_'+kind,'seeded_generator_'+kind)
 Path('work/build_seeded_generator_'+kind+'.cmd').write_text(s)
