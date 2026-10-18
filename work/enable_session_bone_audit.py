from pathlib import Path
p=Path('work/audit_session_model_joints_x64.cpp')
p.write_text(p.read_text().replace('=="joints"','=="bones"'))
