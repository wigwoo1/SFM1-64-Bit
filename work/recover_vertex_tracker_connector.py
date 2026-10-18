from pathlib import Path
import json
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
connector=region(0x100d4c60,0x100d4da9)
factory=region(0x100d5fe0,0x100d604e)
table=[]
for i in range(56):
 address=0x1017b158+i*8;name=im.word(address);cell=im.word(address+4)
 table.append(dict(index=i,address=hex(address),name_address=hex(name),name=im.string(name),output_cell=hex(cell)))
tracker=[x for x in table if x['name']=='VBAllocTracker001']
assert len(tracker)==1 and tracker[0]['output_cell']=='0x10190934'
callers=[]
for i,row in enumerate(rows):
 if bytes.fromhex(row['bytes'])[0]==0xe8:
  raw=bytes.fromhex(row['bytes']);a=int(row['address'],16)
  if a+5+struct.unpack_from('<i',raw,1)[0]==0x100d4c60:
   context=rows[max(0,i-8):i+4]
   for entry in context:
    a=int(entry['address'],16);b=bytes.fromhex(entry['bytes']);assert im.b[im.offset(a):im.offset(a)+len(b)]==b
   callers.append(context)
report=dict(sha256=sha,connector=connector,local_factory_lookup=factory,interface_table=table,tracker=tracker[0],direct_caller_contexts=callers,
policy='Connector takes factory-array/count stack8/c. Bookkeeping count101909c8 selects initialzero branch: for eachfactory each56entry, callfactory(name,nullstatus) onlyifoutputcellzero, store result, record successful outputcell withconnectionindex101909c4. Subsequent nonzero branch alwayscalls/stores eachfactoryresult evennull and retains existingtrackedcellrecords. Negativebookkeeping skipsfactories throughdiagnostic branch. Incrementconnectionindex aftercall. Localfactory traverses registrationlinkedlist exactcase-sensitive name, returns0/status1 ifabsent; foundstatus0 then tailcalls factory.',
limits='Static fullbody/table proof only. Actual suppliedfactoryarray/order, connection state and selectedruntimeprovider unresolved; later connections differ from initial firstnon-null semantics. No concurrent/lifetime or runtime startup claim.',
next_action='Execute connector with controlledfactories and remapped56entrytable/bookkeeping to validate initial/subsequent providerselection, then trace application caller factoryarray.')
Path('outputs/sfm64-vertex-tracker-connector-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_connector_bytes='+str(connector['bytes'])+' factory_lookup_bytes='+str(factory['bytes'])+' table_entries=56 direct_callers='+str(len(callers)))
