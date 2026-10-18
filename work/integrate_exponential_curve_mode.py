from pathlib import Path
p=Path('work/float_curve_evaluation.h');s=p.read_text().replace('#include "bspline_curve_component.h"','#include "bspline_curve_component.h"\n#include "exponential_curve_evaluation.h"').replace('case 11:', 'case 14:return evaluateRecoveredExponentialCurve(times,values,f);\n case 11:');p.write_text(s)
p=Path('work/test_float_curve_sampler.cpp');s=p.read_text().replace('s.info.defaultKind=0xE0E;bool rejected', 's.info.defaultKind=0x1010;bool rejected');p.write_text(s)
