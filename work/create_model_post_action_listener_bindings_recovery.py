from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('ranges=[(0x10390d50,0x10390e4d)]','ranges=[(0x10353b30,0x10353b3f),(0x10353b40,0x10353b4f),(0x10353b50,0x10353b5b)]')
p=p[:p.index('result=dict(')]+'''bindings={124:0x10353b30,125:0x10353b40,126:0x10353b50}
for slot,va in bindings.items():assert im.word(0x10b37094+slot*4)==va
result=dict(complete_code_bytes=41,routines=groups,bindings={str(k):hex(v) for k,v in bindings.items()},findings=[
 'CDataModel slots124/125/126 respectively register/remove/dispatch post-action listeners. All wrappers forward original this+c118 to nested stack; register/remove forward caller pointer unchanged.',
 'Native build must bind corresponding native member rather than apply original x86 c118 byte offset.'
],limitations='Concrete editor listener implementations and production runtime binding pending.')
Path('outputs/model-post-action-listener-bindings-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified model post-action bindings:',41,'bytes; slots124-126')
'''
Path('work/recover_model_post_action_listener_bindings.py').write_text(p)
