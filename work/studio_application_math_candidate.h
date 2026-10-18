#pragma once
#include <stdexcept>
#include <mutex>
#include "studio_owned_math_startup_candidate.h"
#include "studio_native_math_cpu_candidate.h"
struct StudioApplicationMathHostCandidate {
 unsigned cpuFlags(){return studioNativeMathCpuFlagsCandidate();}
 void mathDiagnostic(const char* message){throw std::runtime_error(message);}
};
inline StudioMathTablesOwnerCandidate& studioApplicationMathOwnerCandidate(){static StudioMathTablesOwnerCandidate owner;return owner;}
// All callers complete initialization before reading the shared tables.
// Tables are immutable after startup; generator mutation needs separate ownership.
// Called on the application's main thread before renderer/worker startup.
inline void studioInitializeApplicationMathCandidate(){static std::once_flag once;std::call_once(once,[]{auto& owner=studioApplicationMathOwnerCandidate();StudioApplicationMathHostCandidate host;StudioOwnedMathStartupOpsCandidate<StudioApplicationMathHostCandidate> ops{host,owner};studioMathLibraryInitializeCandidate(owner.startup,2.2f,2.2f,0.0f,2,ops);});}

