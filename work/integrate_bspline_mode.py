from pathlib import Path
p=Path('work/float_curve_evaluation.h');s=p.read_text().replace('#include "normalized_catmull_curve.h"','#include "normalized_catmull_curve.h"\n#include "bspline_curve_component.h"').replace('case 11:', 'case 5:return evaluateRecoveredBSplineComponent(values[0],values[1],values[2],values[3],f);\n case 11:');p.write_text(s)
p=Path('work/test_float_curve_sampler.cpp');s=p.read_text().replace('s.info.defaultKind=0x505;bool rejected', 's.info.defaultKind=0x707;bool rejected');p.write_text(s)
