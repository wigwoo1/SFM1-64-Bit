from pathlib import Path
p=Path('work/recover_owner_reset_cleanup.py');s=p.read_text().replace('(0x10015bd0,0x1001624f)]','(0x10015bd0,0x1001624f),(0x10003990,0x100039a7),(0x100039b0,0x100039c7)]').replace('0x100fa1f4}','0x100fa1f4,0x100fa238}')
s=s.replace("Path('outputs/sfm64-owner-reset-cleanup-recovery.json').write_text", "report['constant_allocator']={'allocation_wrapper':'10003990..100039a7 dereferences imported allocator global100fa238 then slot1(bytes)','free_wrapper':'100039b0..100039c7 same live allocator global slot5(pointer)','limits':'Static exact wrapper/import evidence only, no originalallocator invocation or x64allocatorABIimplementation.'}\nPath('outputs/sfm64-owner-reset-cleanup-recovery.json').write_text")
p.write_text(s)
