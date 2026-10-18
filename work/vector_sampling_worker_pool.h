#pragma once
#include "parallel_vector_sample_batch.h"
#include "stable_graph_sampling_worker_pool.h"
using VectorSamplingWorkerPoolCandidate=StableGraphSamplingWorkerPoolCandidate<VectorSampleJobCandidate,VectorSampleResultCandidate,sampleVectorJobCandidate>;
