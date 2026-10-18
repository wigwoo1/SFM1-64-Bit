from pathlib import Path
import json
records=json.loads(Path('outputs/sfm64-native-procedural-bool-wide-candidates.json').read_text())
record=next(r for r in records if r['address']=='10429E54')
Path('outputs/sfm64-native-procedural-bool-getter-candidate.txt').write_text(record['context']+'\n')
print(record['context'])
