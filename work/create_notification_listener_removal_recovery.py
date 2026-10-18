from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('ranges=[(0x10390d50,0x10390e4d)]','ranges=[(0x102c31d0,0x102c3232),(0x103a37e0,0x103a37f3)]')
p=p[:p.index('result=dict(')]+'''result=dict(complete_code_bytes=117,routines=groups,findings=[
 '103a37e0 removes from nested notification-listener vector28, NOT post-action vector3c.',
 '102c31d0 finds first pointer-equal entry, shifts following entries left with10974fe0 then decrements count; absent returns false. No destructor/free or capacity/cache update.',
 'Duplicates remove only first; null is treated as ordinary pointer. Native move uses pointer-sized elements.'
],limitations='Post-action vector3c removal remains unresolved; notification subscriber UI integration pending.')
Path('outputs/notification-listener-removal-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified notification listener removal:',117,'bytes')
'''
Path('work/recover_notification_listener_removal.py').write_text(p)
