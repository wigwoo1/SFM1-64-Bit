static constexpr unsigned vectorWorkerSequence[]={1,2,4,8,0,4};
#define PREPARED_VECTOR_WORKERS vectorWorkerSequence[frames%6]
#include "test_prepared_timeline_flex_x64.cpp"
