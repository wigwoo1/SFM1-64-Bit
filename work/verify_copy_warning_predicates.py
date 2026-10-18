"""Verify native pre-copy warning predicates without assigning opaque type names."""
import hashlib,json,re,sys
from pathlib import Path
sys.path.insert(0,'work')
from pe_image import Image
binary=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
sha_expected='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4'
im=Image(binary);sha=hashlib.sha256(im.b).hexdigest()
if sha!=sha_expected:raise SystemExit(f'ifm.dll hash changed: {sha}')
dis=Path('work/ifm-disassembly.txt')
def checked_span(start,end):
 rows=[];current=None;entered=False
 for line in dis.open(errors='replace'):
  m=re.match(r'^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)',line)
  if m:
   va=int(m.group(1),16)
   if va<start:continue
   entered=True
   if va>=end:break
   if current:rows.append(current)
   current=[va,bytes.fromhex(m.group(2))]
  elif entered and current and re.match(r'^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$',line):current[1]+=bytes.fromhex(line.strip())
 if current:rows.append(current)
 raw=bytearray();cursor=start
 for va,data in rows:
  if va!=cursor:return None
  try:actual=im.b[im.offset(va):im.offset(va)+len(data)]
  except ValueError:return None
  if actual!=data:return None
  raw.extend(actual);cursor+=len(actual)
 return bytes(raw) if cursor==end else None
ranges={'recursive_control_predicate':(0x105CBF60,0x105CC059),'rig_association_predicate':(0x1072EE00,0x1072EEC3),'override_parent_attribute_check':(0x104C3D50,0x104C3D6A),'typed_attribute_resolver':(0x101E3170,0x101E31F5)}
code={k:checked_span(*v) for k,v in ranges.items()}
a,b,c,d=(code[k] for k in ranges)
facts={
 'control_predicate_scans_two_collection_offsets':bool(a and bytes.fromhex('8B4378') in a and bytes.fromhex('8B4368') in a),
 'control_predicate_recurses_on_nested_items':bool(a and bytes.fromhex('E822FFFFFF') in a),
 'control_predicate_calls_control_resolution_and_override_check':bool(a and bytes.fromhex('E80500F0FF') in a and bytes.fromhex('E86A7DEFFF') in a),
 'override_check_queries_named_overrideParent':bool(c and bytes.fromhex('6868E4B810') in c and bytes.fromhex('E80DF4D1FF') in c),
 'typed_resolver_returns_matching_attribute_handle':bool(d and bytes.fromhex('8B4248FFD0') in d and bytes.fromhex('8B42108BCEFFD0') in d and bytes.fromhex('8BC65E5DC20400') in d),
 'override_check_converts_handle_presence_to_boolean_without_value_read':bool(c and bytes.fromhex('F7D81BC0F7D8') in c),
 'association_predicate_resolves_animationSets':bool(b and bytes.fromhex('E8D189EAFF') in b),
 'association_predicate_scans_resolved_objects':bool(b and bytes.fromhex('E8CF90DAFF') in b and bytes.fromhex('E8349FCCFF') in b),
 'prompt_explicitly_warns_locked_controls_or_rigs':im.string(0x10AA7A90)=='Some of the copied animation sets have locked controls or rigs on them, which could cause problems when pasting them.\nDo you want to continue?'
}
receipt={'binary':str(binary),'sha256':sha,'ranges':{k:{'start':hex(ranges[k][0]),'end_exclusive':hex(ranges[k][1]),'bytes':len(v) if v else 0,'matches_installed_binary':v is not None} for k,v in code.items()},'warning_title':im.string(0x10AA7A74),'warning_body':im.string(0x10AA7A90),'override_attribute_key':im.string(0x10B8E468),'verified_findings':facts,'interpretation_supported':['The Copy AnimationSet(s) callback calls the recursive control predicate with flag 1 and the association predicate before displaying its locked-controls/rigs warning.','The control predicate iterates reference collections, recursively calls itself for nested entries, and has a branch that resolves an associated object and queries the named overrideParent key.','The association predicate resolves an animationSets relationship and examines associated objects through further type/virtual checks.'],'limits':['The opaque runtime type descriptors and virtual-method meanings are not mapped to DMX classes or lock state by this audit.','The overrideParent path checks for a typed attribute handle and converts its presence to a boolean; it does not read the attribute value. The reason this qualifies the native warning remains unresolved.','No live SFM warning scenario was exercised; byte control-flow evidence only.']}
receipt['all_match']=all(v is not None for v in code.values()) and all(facts.values())
Path('outputs/copy-warning-predicate-verification.json').write_text(json.dumps(receipt,indent=2)+chr(10),encoding='utf-8')
print(json.dumps({'sha256':sha,'ranges':receipt['ranges'],'findings':facts,'all_match':receipt['all_match']},indent=2))
if not receipt['all_match']:sys.exit(1)



