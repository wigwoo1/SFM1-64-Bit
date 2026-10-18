from pathlib import Path
p=Path('work/inspect_eye_basis.cpp')
s=p.read_text().replace('auto&p=m.geometry.vertices.at(index).position;','auto p=skinGeometryPositionCandidate(m.geometry.vertices.at(index),palette);')
p.write_text(s)
