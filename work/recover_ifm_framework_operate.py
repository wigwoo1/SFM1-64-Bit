from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');selected=[];pending=None
ranges=[(0x1038F090,0x1038F0B4),(0x1038F0E0,0x1038F0F4),(0x1038F360,0x1038F45A),(0x1038F460,0x1038F46D),(0x10398B80,0x10398BAF),(0x10398C20,0x10398C9E),(0x103996E0,0x103999E5),(0x103999F0,0x10399BC6),(0x10399DA0,0x1039A1B8),(0x1039A1C0,0x1039A258),(0x1076AF00,0x1076B0E0)]
def save(item):
 if item is None:return
 address,data,line=item
 if not any(begin<=address<end for begin,end in ranges):return
 offset=image.offset(address);assert image.b[offset:offset+len(data)]==data
 selected.append(dict(address=hex(address),bytes=data.hex(),instruction=line))
with Path('work/ifm-disassembly.txt').open(encoding='utf-8') as stream:
 for line in stream:
  match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if match:
   save(pending);pending=[int(match[1],16),bytes.fromhex(match[2]),line.strip()]
   if pending[0]>=0x1076B0E0:break
  elif pending:
   tail=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
   if tail:pending[1]+=bytes.fromhex(tail[1])
assert image.word(0x10B3939C+44)==0x1038F360
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),class_name='CDmElementFramework',interface_name=image.string(0x10B39338),vtable='0x10b3939c',slot11='0x1038f360',phase_field='this+0xb4',first_loop_pointer='this+0x50',first_loop_count='this+0x5c',first_loop_virtual_offset=8,second_loop_handle_pointer='this+0x64',second_loop_count='this+0x70',second_loop_virtual_offset=12,verified_complete_instructions=selected,limitations='Candidate installed Operate implementation from matching class/table/query-interface and slot. Global 10E9CCF8 assignment to this implementation and live scheduling remain unverified. Loop virtual method names, graph ordering, dirty filtering and thread-safety require further recovery.')
report['ordering_wrapper']='0x103999f0'
report['selection_routine']='0x103996e0'
report['selection_behavior']='Clear selected count; reset node traversal state and selected byte +0x1c; select nodes for which operator virtual offset 0 returns true and invoke offset 4; also select unselected nodes associated with attributes whose flag word +0x10 includes 0x4000, subject to per-attribute byte +0x18; clear that attribute flag after traversal.'
report['recursive_ordering']='0x103998f0'
report['node_layout_candidate']=dict(state=0,operator=4,output_attribute_nodes_pointer=8,output_attribute_nodes_count=20)
report['ordering_behavior']='Recursive visit with states 0/1/2, cycle flag on state 1, insert node operator after recursive edges, reverse output vector, then invoke each operator virtual offset 0x18 with its list index.'
report['limitations']='Installed instruction evidence only. Framework global assignment, graph construction/edge direction, operator method names, dirty filtering and parallel execution safety remain unverified.'
assert image.word(0x10F01E50)==0x10BD7554 and image.word(0x10F01E54)==0x1076AF00
assert image.word(0x10F01E60)==0x10BD74D4 and image.word(0x10F01E64)==0x1076AFF0
assert image.string(0x10BD7554)=='IDmeOperator_GetInputAttributes'
assert image.string(0x10BD74D4)=='IDmeOperator_GetOutputAttributes'
report['input_attribute_virtual_offset']=16
report['output_attribute_virtual_offset']=20
report['graph_reset']='0x10399da0'
report['graph_profiler_name']=image.string(0x10B39D44)
report['attribute_node_candidate']=dict(attribute_pointer=0,readers_pointer=4,readers_count=16,has_producer_byte=24)
report['edge_direction']='Operator -> output attribute node -> reader operators. Recursive visits readers first, followed by reverse vector order, yielding writer-before-reader ordering for acyclic connected operators.'
report['limitations']='Static graph reset and input/output SWIG method binding evidence. Framework global assignment, attribute-node lookup details, aliases, multiple writers, complete dirty propagation, cycles and parallel execution safety remain unverified.'
report['attribute_identity']='Exact attribute-pointer equality through attribute-node first word; hash is unsigned pointer shifted right by 2. Hash collisions scan the selected bucket and compare equality.'
report['attribute_lookup']='0x10399a80'
report['attribute_compare']='0x10398b80'
report['attribute_hash']='0x10398ba0'
report['bucket_lookup']='0x10398c20'
report['limitations']='Static attribute pointer identity and ordering evidence. Framework global assignment, attribute lifetime/reuse, cross-attribute aliases, multiple writers, complete dirty propagation, cycles and parallel execution safety remain unverified.'
Path('outputs/sfm64-ifm-framework-operate-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified framework instructions:',len(selected))
