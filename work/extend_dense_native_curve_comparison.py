from pathlib import Path
p=Path('work/probe_native_curve_dispatch.cpp');s=p.read_text().replace('unsigned checks=0;','unsigned checks=0;std::vector<float>fractions;for(int i=0;i<=1000;++i)fractions.push_back(static_cast<float>(i)/1000.f);fractions.push_back(-.25f);fractions.push_back(1.25f);').replace('for(float fraction:{0.f,.1f,.25f,.5f,.9f,1.f})','for(float fraction:fractions)');p.write_text(s)
p=Path('work/test_native_curve_dispatch_x64.cpp');s=p.read_text().replace('checks!=1440','checks!=240720').replace('return 0;}catch','return mismatches?2:0;}catch');p.write_text(s)
