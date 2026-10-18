from pathlib import Path
p=Path('work/two_point_curve_evaluation.h');s=p.read_text().replace('#include <cstdint>','#include <cstdint>\n#include <cmath>\n#include <emmintrin.h>');s=s.replace('if(code==4){','''if(code==2||code==3){
  auto angle=_mm_mul_sd(_mm_mul_sd(_mm_cvtss_sd(_mm_setzero_pd(),f),_mm_set_sd(3.141592653589793)),_mm_set_sd(.5));
  if(code==3)angle=_mm_add_sd(angle,_mm_set_sd(1.5707963267948966));
  auto sine=_mm_set_sd(std::sin(_mm_cvtsd_f64(angle)));
  if(code==3)sine=_mm_sub_sd(_mm_set_sd(1.0),sine);
  weight=_mm_cvtsd_ss(_mm_setzero_ps(),sine);
 }
 else if(code==4){''');p.write_text(s)
p=Path('work/float_curve_evaluation.h');s=p.read_text().replace('case 4:case 6:', 'case 2:case 3:case 4:case 6:');p.write_text(s)
p=Path('work/test_two_point_curve_evaluation.cpp');s=p.read_text().replace('bool rejected=false;', '''if(std::abs(evaluateRecoveredTwoPointCurve(2,.5f,0,1)-.70710678f)>1e-7f||std::abs(evaluateRecoveredTwoPointCurve(3,.5f,0,1)-.29289322f)>1e-7f)throw std::runtime_error("sine modes failed");
 for(auto code:{2u,3u})if(std::abs(evaluateRecoveredTwoPointCurve(code,0,0,1))>1e-7f||std::abs(evaluateRecoveredTwoPointCurve(code,1,0,1)-1)>1e-7f)throw std::runtime_error("sine endpoints failed");
 bool rejected=false;''').replace('native_two_point_modes=4,6 checks=10','native_two_point_modes=2,3,4,6 checks=16');p.write_text(s)
