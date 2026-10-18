#pragma once
#include "float_log_schema.h"
// Native base 0x10507650 and float-derived hook 0x105020C0.
// Inherited element name schema and allocator defaults remain separate.
inline constexpr FloatLogSchemaAttribute floatCurveInfoSchemaRecovered[]={
 {"defaultCurveType",2,0},{"minvalue",3,0},{"maxvalue",3,0},
 {"useEdgeInfo",4,0},{"defaultEdgeZeroValue",3,0},{"rightEdgeTime",7,0},
 {"leftEdgeActive",4,0},{"leftEdgeValue",3,0},{"leftEdgeCurveType",2,0},
 {"rightEdgeActive",4,0},{"rightEdgeValue",3,0},{"rightEdgeCurveType",2,0}
};
