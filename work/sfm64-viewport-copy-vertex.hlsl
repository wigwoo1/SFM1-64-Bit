float4 main(uint index : SV_VertexID) : SV_Position {
    const float2 points[3] = {float2(-1, -1), float2(3, -1), float2(-1, 3)};
    return float4(points[index], 0, 1);
}
