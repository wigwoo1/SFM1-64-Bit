from pathlib import Path
p=Path('work/test_timeline_blank_frames.cpp')
s=p.read_text(encoding='utf-8');s=s.replace('!frame.materialInputs.empty()','frame.materialInputs.size()!=0').replace('2237,{},1','2237,{std::filesystem::current_path()},1');p.write_text(s,encoding='utf-8')
