from pathlib import Path
p=Path('work/recover_owner_reset_cleanup.py');s=p.read_text().replace('(0x1002b4c0,0x1002b653)]','(0x1002b4c0,0x1002b653),(0x10028330,0x100283a6),(0x100d74d0,0x100d754e)]')
s=s.replace('0x100fe840]','0x100fe840,0x100fd784,0x100fd774,0x100fda68]')
s=s.replace("Path('outputs/sfm64-owner-reset-cleanup-recovery.json').write_text", "report['post_interface_factory']={'region':'10028330..100283a6','comparison':'100d74d0..100d754e ASCII case-insensitive equality','interfaces':{'ShaderDevice002':'global10183b5c','ShaderApi030':'global10183b4c','ShaderShadow010':'global10183b60'},'return_code':'optional int pointer set0 before lookup, unknown name sets1 and returnsnull; matched name returns live global even ifnull with status0','correction':'callback10028330 is an interface factory; dispatch slot15 receiving it is not proven a deferred scheduling API'}\nPath('outputs/sfm64-owner-reset-cleanup-recovery.json').write_text")
p.write_text(s)
for name in ['studio_owner_post_reset_candidate.h','test_owner_post_reset_candidate.cpp']:
 p=Path('work')/name;p.write_text(p.read_text().replace('schedulePostCallback','publishInterfaceFactory'))
