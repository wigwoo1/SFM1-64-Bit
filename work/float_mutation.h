#pragma once
#include <xmmintrin.h>
// 0x1036D040 suppresses ordered-equal writes; NaNs remain changes.
inline bool floatMutationNeeded(float current,float incoming){return _mm_comieq_ss(_mm_set_ss(current),_mm_set_ss(incoming))==0||_mm_movemask_ps(_mm_cmpunord_ss(_mm_set_ss(current),_mm_set_ss(incoming)))&1;}
