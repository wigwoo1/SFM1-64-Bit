from pathlib import Path
p=Path('work/model_skinning_geometry.h');s=p.read_text().replace('const std::vector<GeometryMatrixCandidate>&locals){','const std::vector<GeometryMatrixCandidate>&locals,std::vector<GeometryMatrixCandidate>*outGlobals=nullptr){').replace(' return palette;', ' if(outGlobals)*outGlobals=std::move(globals);return palette;');p.write_text(s)
src=Path('work/build_body_selection_test.cmd').read_text().replace('test_model_body_selection','test_eye_world');Path('work/build_eye_world_test.cmd').write_text(src)
