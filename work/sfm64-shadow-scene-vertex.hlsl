struct ShadowPush { float4 row0;float4 row1;float4 row2;float4 row3; };
[[vk::push_constant]] ConstantBuffer<ShadowPush> settings;
float4 main(float3 position:POSITION):SV_Position {
 float4 world=float4(position,1);
 float4 projected=float4(dot(settings.row0,world),dot(settings.row1,world),dot(settings.row2,world),dot(settings.row3,world));
 return float4(2*projected.x-projected.w,2*projected.y-projected.w,projected.z,projected.w);
}
