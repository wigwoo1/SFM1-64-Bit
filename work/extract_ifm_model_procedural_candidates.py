from pathlib import Path
import json
records=json.loads(Path('outputs/sfm64-ifm-procedural-bool-candidates.json').read_text());selected=[r for r in records if r['address'] in ('104372CD','1072CB70')];assert len(selected)==2
text='\n\n'.join(r['context'] for r in selected)+'\n';Path('outputs/sfm64-ifm-model-procedural-candidates.txt').write_text(text);print(text)
