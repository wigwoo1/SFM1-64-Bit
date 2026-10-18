from pathlib import Path
s=Path('work/probe_native_vector_replay.cpp').read_text().replace('redirect(0x10372db0','redirect(0x1036d270').replace('i<3','i<4').replace('array<uint32_t,13>record','array<uint32_t,15>record').replace('record[10+i]','record[11+i]').replace('0x10378ad0:0x10378b20','0x10377f80:0x10377fd0').replace('native_vector_replay_cases','native_quaternion_replay_cases')
Path('work/probe_native_quaternion_replay.cpp').write_text(s)
s=Path('work/test_native_vector_replay_x64.cpp').read_text().replace('Candidate<3>','Candidate<4>').replace('i<3','i<4').replace('array<float,3>','array<float,4>').replace('array<uint32_t,3>','array<uint32_t,4>').replace('v.data(),12','v.data(),16').replace('native vector replay mismatch','native quaternion replay mismatch').replace('x64_vector_replay_cases','x64_quaternion_replay_cases')
Path('work/test_native_quaternion_replay_x64.cpp').write_text(s)
