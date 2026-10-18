#pragma once
#include "parallel_scalar_sample_batch.h"
#include "stable_graph_sampling_worker_pool.h"
using ScalarSamplingWorkerPoolCandidate=StableGraphSamplingWorkerPoolCandidate<ScalarSampleJobCandidate,ScalarSampleResultCandidate,sampleScalarJobCandidate>;
