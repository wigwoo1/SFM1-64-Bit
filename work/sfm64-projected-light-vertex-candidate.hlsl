struct CameraRows {float4 row0;float4 row1;float4 row2;float4 row3;};
[[vk::push_constant]] ConstantBuffer<CameraRows> camera;
struct Input {float3 position:POSITION;float3 normal:NORMAL;float2 uv:TEXCOORD0;};
struct Output {float4 position:SV_Position;float2 uv:TEXCOORD0;float3 worldNormal:TEXCOORD1;float3 worldPosition:TEXCOORD2;};
Output main(Input input){Output o;float4 p=float4(input.position,1);o.position=float4(dot(camera.row0,p),dot(camera.row1,p),dot(camera.row2,p),dot(camera.row3,p));o.uv=input.uv;o.worldNormal=input.normal;o.worldPosition=input.position;return o;}
