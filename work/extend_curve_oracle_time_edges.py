from pathlib import Path
p=Path('work/probe_native_curve_components.cpp');s=p.read_text();s=s.replace('std::array<float,4>{0,1,3,7}}){std::array<float,4>times{0,1,3,7};', 'std::array<float,4>{0,1,3,7}})for(auto times:{std::array<float,4>{0,1,3,7},std::array<float,4>{0,1,2,3},std::array<float,4>{1,1,3,3},std::array<float,4>{0,1,1,7},std::array<float,4>{100000000.f,100000008.f,100000024.f,100000056.f}}){');p.write_text(s)
p=Path('work/test_native_curve_oracle_x64.cpp');s=p.read_text().replace('checks!=144','checks!=720');p.write_text(s)
