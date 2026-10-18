from pathlib import Path
s=Path('work/probe_native_curve_components.cpp').read_text();s=s.replace('#include "catmull_curve_component.h"','#include "float_curve_evaluation.h"\n#include <limits>\n#include "catmull_curve_component.h"').replace('mode<8','mode<16')
a=s.index('if(mode<5){');b=s.index('oracle.write',a)
s=s[:a]+'''using Dispatch=void(__cdecl*)(uint32_t,const float*,const float*,const float*,const float*,float,float*);reinterpret_cast<Dispatch>(0x100669D0)(mode,points[0].data(),points[1].data(),points[2].data(),points[3].data(),fraction,out.data());'''+s[b:]
a=s.index('float replacement=');b=s.index('++checks;',a)
s=s[:a]+'''++checks;'''+s[b+len('++checks;'):]
s=s.replace(' bitwise_equal=1 DLL_entrypoint_not_called=1',' oracle_generated=1 DLL_entrypoint_not_called=1');Path('work/probe_native_curve_dispatch.cpp').write_text(s)
s=Path('work/test_native_curve_oracle_x64.cpp').read_text().replace('#include "catmull_curve_component.h"','#include "float_curve_evaluation.h"\n#include <limits>\n#include "catmull_curve_component.h"').replace('mode>7','mode>15').replace('checks!=720','checks!=1440').replace('unsigned checks=0;','unsigned checks=0,mismatches=0;std::array<unsigned,16>byMode{};')
a=s.index('float actual=');b=s.index('++checks;',a)
s=s[:a]+'''auto packed=static_cast<int32_t>((mode<<8)|mode);float actual=evaluateRecoveredFloatCurve(times,v,{0,packed,packed,0},fraction,-std::numeric_limits<float>::max(),std::numeric_limits<float>::max());if(std::memcmp(&actual,&expected,4)){++mismatches;++byMode[mode];}'''+s[b:]
s=s.replace('<<" bitwise_equal=1\\n"','<<" mismatches="<<mismatches<<"\\n";for(unsigned i=0;i<16;++i)if(byMode[i])std::cout<<"mode="<<i<<" mismatches="<<byMode[i]<<\'\\n\'')
Path('work/test_native_curve_dispatch_x64.cpp').write_text(s)
