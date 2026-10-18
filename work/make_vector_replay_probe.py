from pathlib import Path
s=Path('work/probe_native_scalar_replay.cpp').read_text()
s=s.replace('redirect(0x10366f90','redirect(0x10372db0').replace('emit(3,*value);','uint32_t hash=2166136261u;for(unsigned i=0;i<3;++i)hash=(hash^value[i])*16777619u;emit(3,hash);')
s=s.replace('std::array<uint32_t,9>record{}','std::array<uint32_t,13>record{}')
s=s.replace('record[8]=(seed&32)?0xffffffffu:seed+200;','for(unsigned i=0;i<3;++i){record[7+i]=(seed&16)?0xffffffffu:seed+100+i;record[10+i]=(seed&32)?0xffffffffu:seed+200+i;}')
s=s.replace('0x1036c190:0x1036c1f0','0x10378ad0:0x10378b20').replace('native_scalar_replay_cases','native_vector_replay_cases')
Path('work/probe_native_vector_replay.cpp').write_text(s)
