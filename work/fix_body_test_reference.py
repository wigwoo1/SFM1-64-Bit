from pathlib import Path
p=Path('work/test_model_body_selection.cpp');s=p.read_text().replace('body/6};','(body/6)%5};');p.write_text(s)
