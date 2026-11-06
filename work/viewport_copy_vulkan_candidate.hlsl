[[vk::binding(0, 0)]] Texture2D<float4> sourceImage;

float4 copyVertex(uint vertex : SV_VertexID) : SV_Position {
    const float2 points[3] = {float2(-1, -1), float2(3, -1), float2(-1, 3)};
    return float4(points[vertex], 0, 1);
}

float4 copyFragment(float4 position : SV_Position) : SV_Target0 {
    return sourceImage.Load(int3(int2(position.xy), 0));
}
