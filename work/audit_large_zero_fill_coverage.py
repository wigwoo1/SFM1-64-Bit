from pathlib import Path
import json
# Store-span model of checked 10465C0E..10465CC7. Counts >=128 enter this path.
cases=0
for offset in range(16):
 for count in range(128,4097):
  cursor=offset;remaining=count;spans=[]
  def store(width):
   global cursor
   spans.append((cursor,cursor+width));cursor+=width
  if offset:
   alignment=16-offset;remaining-=alignment
   for _ in range(alignment&3):store(1)
   for _ in range(alignment>>2):store(4)
  for _ in range(remaining>>7):
   for _ in range(8):store(16)
  remaining&=127
  for _ in range(remaining>>4):store(16)
  remaining&=15
  for _ in range(remaining>>2):store(4)
  for _ in range(remaining&3):store(1)
  assert spans[0][0]==offset and spans[-1][1]==offset+count
  assert all(left[1]==right[0] for left,right in zip(spans,spans[1:]))
  assert all(begin%16==0 for begin,end in spans if end-begin==16)
  cases+=1
report=dict(cases=cases,all_store_spans_exact=True,all_vector_stores_aligned=True,scope='Independent store-span model for all 16 alignments and counts 128..4096; no execution of native routine or allocator failure oracle')
Path('outputs/sfm64-large-zero-fill-coverage.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('Verified zero-fill span/alignment cases:',cases)
