// HDR sample accumulation. Inputs and output are linear float32 RGBA.
[[vk::binding(0, 0)]] StructuredBuffer<float4> currentSamples;
[[vk::binding(1, 0)]] StructuredBuffer<float4> previousAverage;
[[vk::binding(2, 0)]] RWStructuredBuffer<float4> nextAverage;

struct AccumulationParameters {
    uint sampleOffset;
    uint pixelCount;
    float currentWeight;
    float previousWeight;
};
[[vk::push_constant]] AccumulationParameters params;

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint pixel = id.x;
    if (pixel >= params.pixelCount)
        return;
    precise float4 history = previousAverage[pixel] * params.previousWeight;
    nextAverage[pixel] = mad(currentSamples[params.sampleOffset + pixel], params.currentWeight, history);
}
