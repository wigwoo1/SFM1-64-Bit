from pathlib import Path
p=Path('work/recover_editor_time_adjustment.py').read_text()
p=p.replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x1097af20,0x1097b012),(0x10a16680,0x10a1672a),(0x10a16af0,0x10a16b24)]').replace('groups=[[],[]]','groups=[[],[],[]]')
p=p[:p.index('result=dict')]+'''result=dict(complete_code_bytes=sum(e-s for s,e in ranges),routines=groups,findings=[
 '1097af20 multiplies signed32 time by unsigned16 numerator through low64 multiply helper10a16af0; denominator is unsigned16 units.',
 'For negative time mode0 subtract units/2 then add numerator; mode1 subtract units then add numerator; other modes leave product unchanged.',
 'For nonnegative time mode0 adds units/2; mode2 subtracts numerator then adds units; modes1/3 add numerator; other modes leave product unchanged.',
 'All paths signed-divide by units via10a16680, return low32 quotient. Preserve literal adjustments rather than replacing with generic floor/ceil.'
],limitations='Complete bytes verified; zero denominator faults and original execution parity remain pending. Native candidate will restrict denominator to nonzero.')
Path('outputs/sfm-time-to-frame-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified integer conversion/helper bytes:',result['complete_code_bytes'])
'''
Path('work/recover_sfm_time_to_frame.py').write_text(p)
