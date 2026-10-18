from pathlib import Path
import json
records=json.loads(Path('outputs/sfm64-viewport-procedural-access-candidates.json').read_text());selected=[r for r in records if r['address'] in ('101A4D20','101A4ECA')]
assert len(selected)==2
text='\n\n'.join(r['context'] for r in selected)+'\n';Path('outputs/sfm64-viewport-procedural-branches.txt').write_text(text);print(text)
