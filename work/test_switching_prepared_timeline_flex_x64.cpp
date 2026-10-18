static constexpr unsigned switchingScalarWorkers[]={4,1,2,8,0,4};
#define PREPARED_SCALAR_WORKERS switchingScalarWorkers[frames%6]
#include "test_prepared_timeline_flex_x64.cpp"
