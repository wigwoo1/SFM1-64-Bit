struct CameraRows { float4 row0; float4 row1; float4 row2; float4 row3; };
[[vk::push_constant]] ConstantBuffer<CameraRows> camera;
struct Input {
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 normal : NORMAL;
    [[vk::location(2)]] float2 uv : TEXCOORD0;
    [[vk::location(3)]] float displacementAlpha : TEXCOORD1;
    [[vk::location(4)]] float2 lightUv : TEXCOORD2;
};
struct Output {
    float4 position : SV_Position;
    [[vk::location(0)]] float2 uv : TEXCOORD0;
    [[vk::location(1)]] float3 vertexLight : TEXCOORD1;
    [[vk::location(2)]] float displacementAlpha : TEXCOORD2;
    [[vk::location(3)]] float2 lightUv : TEXCOORD3;
};
Output main(Input v) {
    Output o;float4 p=float4(v.position,1);
    o.position=float4(dot(camera.row0,p),dot(camera.row1,p),dot(camera.row2,p),dot(camera.row3,p));
    o.uv=v.uv;o.vertexLight=v.normal;o.displacementAlpha=v.displacementAlpha;o.lightUv=v.lightUv;return o;
}
