#define main dispatchFixtureMain
#include "test_restore_dispatch_native.cpp"
#undef main
int main(){std::puts("dispatch_fixture_entry_reached");std::fflush(stdout);return 0;}
