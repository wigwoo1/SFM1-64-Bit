from pathlib import Path
p=Path('work/probe_native_tick_seconds_x86.cpp');s=p.read_text().replace('mapped[0x15fb73]=char(0xc3);','reinterpret_cast<unsigned char*>(mapped)[0x15fb73]=0xc3;').replace('__asm{lea edx,tick mov eax,native call eax movss seconds,xmm0}','__asm {\n lea edx,tick\n mov eax,native\n call eax\n movss seconds,xmm0\n }');p.write_text(s)
