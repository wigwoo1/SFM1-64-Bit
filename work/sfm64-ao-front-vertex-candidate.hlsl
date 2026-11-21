struct ViewProjection { float4 x; float4 y; float4 z; float4 w; };
[[vk::push_constant]] ConstantBuffer<ViewProjection> camera;

struct Input {
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 normal : NORMAL;
    [[vk::location(2)]] float2 uv : TEXCOORD0;
};

struct Output {
    float4 position : SV_Position;
    [[vk::location(0)]] float3 worldNormal : TEXCOORD0;
    [[vk::location(1)]] float2 uv : TEXCOORD1;
};

Output main(Input input) {
    float4 world = float4(input.position, 1);
    Output result;
    result.position = float4(dot(camera.x, world), dot(camera.y, world),
                             dot(camera.z, world), dot(camera.w, world));
    result.worldNormal = input.normal;
    result.uv = input.uv;
    return result;
}
