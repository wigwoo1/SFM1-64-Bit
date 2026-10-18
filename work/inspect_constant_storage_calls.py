import json
r=json.load(open('outputs/sfm64-owner-reset-cleanup-recovery.json'))
rows=[x for x in r['instructions'] if 0x10015bd0<=int(x['address'],16)<0x1001624f]
for index,row in enumerate(rows):
 if row['instruction'].startswith('call'):
  print('\n'+row['address']+' '+row['instruction'])
  for prev in rows[max(0,index-4):index]:print(prev['instruction'])
  for nxt in rows[index+1:index+3]:print(nxt['instruction'])
