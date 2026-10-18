from pathlib import Path
for name in ['probe_native_curve_components.cpp','test_native_curve_oracle_x64.cpp']:
 p=Path('work')/name;s=p.read_text().replace('#include "chord_catmull_curve.h"','#include "chord_catmull_curve.h"\n#include "parameterized_curve_component.h"\ninline float oracleParameterized(const std::array<float,4>&times,const std::array<float,4>&v,float fraction,unsigned mode){auto n=normalizeRecoveredCurveValues(times,v);return evaluateRecoveredParameterizedComponent(n[0],n[1],n[2],n[3],fraction,recoveredParameterizedCurve(mode+2));}')
 if name.startswith('probe'):
  s=s.replace('mode<5','mode<8')
  call='reinterpret_cast<Native>(addresses[mode])(points[0].data(),points[1].data(),points[2].data(),points[3].data(),fraction,out.data());'
  s=s.replace(call,'''if(mode<5){'''+call+'''}else{using ParamNative=void(__cdecl*)(float,float,float,const float*,const float*,const float*,const float*,float,float*);auto p=recoveredParameterizedCurve(mode+2);reinterpret_cast<ParamNative>(0x1049B7E0)(p.first,p.second,p.third,points[0].data(),points[1].data(),points[2].data(),points[3].data(),fraction,out.data());}''')
 else:s=s.replace('mode>4','mode>7').replace('checks!=90','checks!=144')
 s=s.replace(':evaluateRecoveredChordCatmull(times,v,fraction)',':mode==4?evaluateRecoveredChordCatmull(times,v,fraction):oracleParameterized(times,v,fraction,mode)');p.write_text(s)
