#pragma once
#include <optional>
#include <mutex>
#include <atomic>
#include <stdexcept>
#include "native_cpu_topology_candidate.h"
#include "native_cpu_mask_services_candidate.h"
#include "studio_cpu_count_publication_candidate.h"
struct NativeCpuCountServicesCandidate{
 NativeCpuMaskServicesCandidate snapshot;
 unsigned logicalCount(){SYSTEM_INFO info{};GetSystemInfo(&info);return info.dwNumberOfProcessors;}
 bool isAMD(){const auto& value=snapshot.basic;return value[1]==0x68747541u&&value[2]==0x444d4163u&&value[3]==0x69746e65u;}
 unsigned backendCount(){NativeCpuTopologyQueryCandidate query;auto result=nativeCpuTopologyCandidate(query);if(!result.available)throw std::runtime_error("legacy topology unavailable; CPU fallback binding pending");return result.cores;}
};
// Count publication is synchronized. Use atomic-mode alias for concurrent
// mode writes. Full original CPU record/cache lifecycle/fallback external.
template<class Mode>struct NativeCpuCreationFlagServicesCandidateT{
 Mode& mode;std::optional<StudioCpuCountPublicationCandidate> counts;std::once_flag initialization;
 std::atomic<unsigned> cpuCalls{0},initializations{0};
 int queueMode(){return mode;}
 unsigned cpuByte6(){++cpuCalls;std::call_once(initialization,[&]{NativeCpuCountServicesCandidate services;auto value=studioCpuCountPublicationCandidate(services);counts=value;++initializations;});return counts->physical;}
};
using NativeCpuCreationFlagServicesCandidate=NativeCpuCreationFlagServicesCandidateT<int>;
using NativeAtomicCpuCreationFlagServicesCandidate=NativeCpuCreationFlagServicesCandidateT<std::atomic<int>>;
