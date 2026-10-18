import json
r=json.load(open('outputs/sfm64-owner-reset-cleanup-recovery.json'))
for row in r['instructions']:
 a=int(row['address'],16);i=row['instruction']
 if 0x1000e550<=a<0x1000ea5f and not i.startswith(('push','call','mov         ecx','pop')):print(row['address'],i)
