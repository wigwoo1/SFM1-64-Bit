from pathlib import Path
p=Path('work/test_scene_material_inputs.cpp');s=p.read_text().replace('inputs[0].materials[n].source.vmt.stem().string()=="visor"','vmtAsciiLowerCandidate(inputs[0].materials[n].source.vmt.stem().string())=="visor"');p.write_text(s)
