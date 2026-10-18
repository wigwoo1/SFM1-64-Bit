from pathlib import Path
s=Path('work/recover_editor_time_adjustment.py').read_text().replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x10353bc0,0x10353be1),(0x10390690,0x10390695),(0x103906a0,0x103906a5)]').replace('groups=[[],[]]','groups=[[],[],[]]')
s=s[:s.index('result=dict')]+'''assert im.word(0x10b37094+0xf0)==0x10353bc0
result=dict(complete_code_bytes=43,routines=groups,findings=[
 'Model60 setter10353bc0 testsentireargumentbyte nonzero, notlowbit; delegatesnestedthis+c118 helpers bit3 OR8 orANDf7 atnested74/modelc18c.',
 'Any nonzero byte enablesrecording; zero disables. Other bits includingnotificationsuppressionbit4 preserved.'
],limitations='Completebytesverified; originalexecutionparity/replaycaller toggles/nativeundoqueuebindingpending.')
Path('outputs/model-undo-enable-setter-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified undo enable setter:',43,'bytes')
'''
Path('work/recover_model_undo_enable_setter.py').write_text(s)
p=Path('work/model_undo_record_gate_candidate.h');s=p.read_text()+'''\n// Model60 normalizes any nonzero input byte, unlike suppression setter bit0.
inline void setModelUndoRecordEnabledCandidate(std::uint8_t& flagsC18c,std::uint8_t incoming) {
    flagsC18c=static_cast<std::uint8_t>((flagsC18c&0xf7u)|(incoming?8u:0u));
}
''';p.write_text(s)
