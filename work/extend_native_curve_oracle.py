from pathlib import Path
for name in ['probe_native_curve_components.cpp','test_native_curve_oracle_x64.cpp']:
 p=Path('work')/name;s=p.read_text().replace('#include "catmull_derivative_curve.h"','#include "catmull_derivative_curve.h"\n#include "normalized_catmull_curve.h"\n#include "chord_catmull_curve.h"')
 if name.startswith('probe'):
  s=s.replace('std::array<uintptr_t,3>addresses{0x1049A500,0x1049BA80,0x1049A7C0}', 'std::array<uintptr_t,5>addresses{0x1049A500,0x1049BA80,0x1049A7C0,0x1049AC20,0x1049AA70}').replace('mode<3','mode<5')
  s=s.replace('std::array<std::array<float,3>,4>points{};for(unsigned i=0;i<4;++i)points[i]={float(i),v[i],0};','std::array<float,4>times{0,1,3,7};std::array<std::array<float,3>,4>points{};for(unsigned i=0;i<4;++i)points[i]={times[i],v[i],0};')
  s=s.replace('oracle.write(reinterpret_cast<const char*>(&out[1]),4);','oracle.write(reinterpret_cast<const char*>(times.data()),16);oracle.write(reinterpret_cast<const char*>(&out[1]),4);')
 else:
  s=s.replace('std::array<float,4>v;', 'std::array<float,4>v,times;').replace('mode>2','mode>4').replace('f.read(reinterpret_cast<char*>(&expected),4);', 'f.read(reinterpret_cast<char*>(times.data()),16);f.read(reinterpret_cast<char*>(&expected),4);').replace('checks!=54','checks!=90')
 s=s.replace(':evaluateRecoveredCatmullDerivative(v[0],v[1],v[2],v[3],fraction)', ':mode==2?evaluateRecoveredCatmullDerivative(v[0],v[1],v[2],v[3],fraction):mode==3?evaluateRecoveredNormalizedCatmull(times,v,fraction):evaluateRecoveredChordCatmull(times,v,fraction)')
 p.write_text(s)
