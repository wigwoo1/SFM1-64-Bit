#define NOMINMAX
#ifdef PROBE_WINDOWS
#include <Windows.h>
#endif
#include <array>
#ifdef PROBE_FSTREAM
#include <fstream>
#endif
#include <cstdio>
#ifdef PROBE_REGISTRY
#include "restore_dispatch_native_init.h"
#include "native_manager_callback_registry_candidate.h"
#endif
int main(){std::puts("dispatch_headers_entry_reached");return 0;}
