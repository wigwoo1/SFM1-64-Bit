from pathlib import Path
p=Path('work/two_point_curve_evaluation.h');s=p.read_text().replace('auto f=_mm_set_ss(fraction);__m128 weight;', '''auto f=_mm_set_ss(fraction);__m128 weight;
 if(code==10){auto square=_mm_mul_ss(f,f),cube=_mm_mul_ss(square,f);auto a=_mm_set_ss(first),b=_mm_set_ss(next);
  auto result=_mm_add_ss(_mm_mul_ss(b,_mm_mul_ss(cube,_mm_set_ss(-2))),_mm_mul_ss(a,_mm_mul_ss(cube,_mm_set_ss(2))));
  result=_mm_add_ss(result,_mm_mul_ss(a,_mm_mul_ss(square,_mm_set_ss(-3))));
  result=_mm_add_ss(result,_mm_mul_ss(b,_mm_mul_ss(square,_mm_set_ss(3))));return _mm_cvtss_f32(_mm_add_ss(a,result));
 }''');p.write_text(s)
p=Path('work/float_curve_evaluation.h');s=p.read_text().replace('case 4:case 6:', 'case 4:case 6:case 10:');p.write_text(s)
p=Path('work/test_float_curve_sampler.cpp');s=p.read_text().replace('s.info.defaultKind=0xA0A;bool rejected','s.info.defaultKind=0xC0C;bool rejected');p.write_text(s)
p=Path('work/test_two_point_curve_evaluation.cpp');s=p.read_text().replace('bool rejected=false;','''for(float f:{0.f,.25f,.5f,.75f,1.f})if(std::abs(evaluateRecoveredTwoPointCurve(10,f,2,10)-evaluateRecoveredTwoPointCurve(4,f,2,10))>1e-6f)throw std::runtime_error("mode10 polynomial failed");
 bool rejected=false;''');p.write_text(s)
