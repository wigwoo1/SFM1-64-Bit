#pragma once
#include <cmath>
#include <xmmintrin.h>
#include <emmintrin.h>
#include "studio_math_library_startup_candidate.h"
#include "studio_trigonometric_table_candidate.h"
#include "studio_gamma_startup_candidate.h"
#include "studio_seeded_generator_candidate.h"
struct StudioMathTablesOwnerCandidate {
 StudioMathLibraryStartupCandidate startup;
 std::array<float,256> trigonometric{};
 StudioGammaTablesCandidate gamma;
 StudioSeedStateCandidate seeded;
 StudioMathTablesOwnerCandidate()=default;
 StudioMathTablesOwnerCandidate(const StudioMathTablesOwnerCandidate&)=delete;
 StudioMathTablesOwnerCandidate& operator=(const StudioMathTablesOwnerCandidate&)=delete;
 StudioMathTablesOwnerCandidate(StudioMathTablesOwnerCandidate&&)=delete;
 StudioMathTablesOwnerCandidate& operator=(StudioMathTablesOwnerCandidate&&)=delete;
};
// Native output parity is established for tested finite startup inputs with
// default nearest rounding. Final rounded conversion follows MXCSR here;
// a separately configured x87 rounding mode is not yet reproduced.
struct StudioStartupMathArithmeticCandidate {
 double sine(double x){return std::sin(x);}
 float powerFloat(float x,float y){return std::pow(x,y);}
 double powerDouble(double x,double y){return std::pow(x,y);}
 int truncateFloat(float x){return _mm_cvttss_si32(_mm_set_ss(x));}
 int truncateDouble(double x){return _mm_cvttsd_si32(_mm_set_sd(x));}
 int roundFloat(float x){return _mm_cvtss_si32(_mm_set_ss(x));}
};
template<class Host>struct StudioOwnedMathStartupOpsCandidate {
 Host& host;StudioMathTablesOwnerCandidate& owner;
 unsigned cpuFlags(){return host.cpuFlags();}
 void diagnostic(const char* message){host.mathDiagnostic(message);}
 void initializeTrigonometricTable(){StudioStartupMathArithmeticCandidate arithmetic;const std::array<double,3> constants{2.0,3.1415926535897931,0.00390625};studioInitializeTrigonometricTableCandidate(owner.trigonometric,constants,arithmetic);}
 void buildGammaTables(float a,float b,float c,int d){StudioStartupMathArithmeticCandidate arithmetic;studioGammaStartupCandidate(owner.gamma,a,b,c,d,arithmetic);}
 void seedGenerator(unsigned seed){studioInitializeSeedCandidate(owner.seeded,seed);}
};
template<class Host>struct StudioMathBoundStartupHostCandidate {
 Host& host;StudioMathTablesOwnerCandidate& math;
 void diagnostic(unsigned count){host.diagnostic(count);}
 void initializeCvarAccess(){host.initializeCvarAccess();}
 void registerCvars(unsigned flags){host.registerCvars(flags);}
 void warning(const char* message){host.warning(message);}
 void initializeMathLibrary(float a,float b,float c,int d,int,int,int,int){StudioOwnedMathStartupOpsCandidate<Host> ops{host,math};studioMathLibraryInitializeCandidate(math.startup,a,b,c,d,ops);}
};
