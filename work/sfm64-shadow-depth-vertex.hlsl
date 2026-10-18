struct ShadowPush { float depth; float slope; };
[[vk::push_constant]] ConstantBuffer<ShadowPush> settings;
float4 main(uint vertex:SV_VertexID):SV_Position {
 float2 xy[3]={float2(-1,-1),float2(3,-1),float2(-1,3)};
 return float4(xy[vertex],settings.depth+settings.slope*(xy[vertex].x+1)*0.5,1);
}
