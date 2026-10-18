from pathlib import Path
p=Path('work/inspect_session_render_dimensions.cpp');s=p.read_text();s=s.replace('}return 0;}catch','}DimensionAttributeAudit::run(g);return 0;}catch');p.write_text(s)
