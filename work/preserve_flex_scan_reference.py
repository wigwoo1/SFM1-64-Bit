from pathlib import Path
root=Path(__file__).parent
source=(root/'owned_global_flex_binding_plan.h').read_text()
start=source.index('template<class Projection,class ResolveMetadata>')
reference=source[start:].replace('discoverOwnedGlobalFlexBindingsCandidate','discoverOwnedGlobalFlexBindingsNaiveReference',1)
target=root/'reference_global_flex_binding_scan.h'
assert not target.exists()
target.write_text('#pragma once\n#include "owned_global_flex_binding_plan.h"\n// Prior whole-graph scan retained solely as independent optimization reference.\n'+reference)
