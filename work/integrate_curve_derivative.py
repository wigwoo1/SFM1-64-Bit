from pathlib import Path
p=Path('work/float_curve_evaluation.h');s=p.read_text().replace('#include "bspline_curve_component.h"','#include "bspline_curve_component.h"\n#include "catmull_derivative_curve.h"').replace('case 11:', 'case 13:return evaluateRecoveredCatmullDerivative(values[0],values[1],values[2],values[3],f);\n case 11:');p.write_text(s)
p=Path('work/test_float_curve_sampler.cpp');s=p.read_text().replace('s.info.defaultKind=0xD0D;bool rejected','s.info.defaultKind=0xE0E;bool rejected');p.write_text(s)
