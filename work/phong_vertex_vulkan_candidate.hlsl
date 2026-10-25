// Owned bounded Phong world-frame core. No material/pixel-lighting behavior.
// Geometric directions are separate inputs, never BSP baked-RGB normal fields.
[[vk::binding(0,0)]] cbuffer SkinConstants {
    float4 paletteRows[159];
    float4 flexScale;
};
struct WorldFrame {
    float4 clip:SV_Position;
    [[vk::builtin("PointSize")]] float pointSize:PSIZE;
    [[vk::location(0)]] float3 position:TEXCOORD0;
    [[vk::location(1)]] float3 normal:TEXCOORD1;
    [[vk::location(2)]] float3 tangent:TEXCOORD2;
    [[vk::location(3)]] float3 binormal:TEXCOORD3;
};
struct PlainInput {
    [[vk::location(0)]] float3 position:POSITION0;
    [[vk::location(1)]] float2 weights:BLENDWEIGHT0;
    [[vk::location(2)]] float4 indices:BLENDINDICES0;
    [[vk::location(3)]] float3 normal:NORMAL0;
    [[vk::location(4)]] float4 tangent:TANGENT0;
    [[vk::location(5)]] float4 positionDelta:POSITION1;
    [[vk::location(6)]] float3 directionDelta:NORMAL1;
};
struct PackedInput {
    [[vk::location(0)]] float3 position:POSITION0;
    [[vk::location(1)]] int2 weights:BLENDWEIGHT0;
    [[vk::location(2)]] float4 indices:BLENDINDICES0;
    [[vk::location(3)]] float4 frame:NORMAL0;
    [[vk::location(5)]] float4 positionDelta:POSITION1;
    [[vk::location(6)]] float3 directionDelta:NORMAL1;
};
float dot3Reference(float3 a,float3 b){
    precise float x=a.x*b.x,y=a.y*b.y,z=a.z*b.z;
    precise float pair=x+y,result=pair+z;return result;
}
float dot4Reference(float4 a,float4 b){
    precise float x=a.x*b.x,y=a.y*b.y,z=a.z*b.z,w=a.w*b.w;
    precise float pair=x+y,xyz=pair+z,result=xyz+w;return result;
}
float3 normalizeReference(float3 value){
    precise float lengthSquared=dot3Reference(value,value);
    precise float root=sqrt(lengthSquared),inverse=1.0f/root;
    precise float3 result=value*inverse;return result;
}
float3 decodeDirection(float2 bytes){
    precise float2 centered=bytes-128.0f;
    float2 negative=float2(centered.x<0?1.0f:0.0f,centered.y<0?1.0f:0.0f);
    precise float2 folded=abs(centered)-negative,shifted=folded-64.0f;
    float2 negativeAxis=float2(shifted.x<0?1.0f:0.0f,shifted.y<0?1.0f:0.0f);
    precise float2 distance=abs(shifted)-negativeAxis;
    precise float2 signs=negativeAxis*(-2.0f)+1.0f;
    const float scale=1.0f/63.0f;
    precise float z0=distance.x*(-scale),z1=z0+1.0f,z2=distance.y*(-scale),z=z2+z1;
    precise float3 n=normalizeReference(float3(distance*scale,z));
    precise float3 result=float3(signs*n.xy,(bytes.x<128.0f?-1.0f:1.0f)*n.z);return result;
}
float4 blendRow(float4 m0,float4 m1,float4 m2,float2 weights,float residual){
    precise float4 second=weights.y*m1,first=m0*weights.x,pair=first+second;
    precise float4 third=m2*residual,result=third+pair;return result;
}
WorldFrame skinFrame(float3 position,float2 weights,float4 indices,float3 normal,float3 tangent,float handedness,float3 pDelta,float3 dDelta){
    // R8G8B8A8_UNORM inputs preserve stored hardware-ID order; native D3DCOLOR
    // swizzles BGR and then the shader swizzles back. This is its XYZ equivalent.
    uint3 rows=(uint3)round(indices.xyz*765.005859375f);
    precise float sum=weights.y+weights.x,residual=1.0f-sum;
    precise float4 r0=blendRow(paletteRows[rows.x],paletteRows[rows.y],paletteRows[rows.z],weights,residual);
    precise float4 r1=blendRow(paletteRows[rows.x+1],paletteRows[rows.y+1],paletteRows[rows.z+1],weights,residual);
    precise float4 r2=blendRow(paletteRows[rows.x+2],paletteRows[rows.y+2],paletteRows[rows.z+2],weights,residual);
    precise float3 pd=pDelta*flexScale.x,dd=dDelta*flexScale.x;
    precise float4 p=float4(pd+position,1.0f);
    precise float3 n=dd+normal,t=dd+tangent;
    precise float3 rawN=float3(dot3Reference(n,r0.xyz),dot3Reference(n,r1.xyz),dot3Reference(n,r2.xyz));
    precise float3 rawT=float3(dot3Reference(t,r0.xyz),dot3Reference(t,r1.xyz),dot3Reference(t,r2.xyz));
    precise float3 negative=rawN.zxy*rawT.yzx,positive=rawN.yzx*rawT.zxy;
    precise float3 difference=positive+(-negative),rawB=difference*handedness;
    WorldFrame o;o.clip=float4(0.25f,0.25f,0.5f,1.0f);o.pointSize=1.0f;
    o.position=float3(dot4Reference(p,r0),dot4Reference(p,r1),dot4Reference(p,r2));
    o.normal=normalizeReference(rawN);o.tangent=normalizeReference(rawT);o.binormal=normalizeReference(rawB);return o;
}
WorldFrame plainMain(PlainInput i){return skinFrame(i.position,i.weights,i.indices,i.normal,i.tangent.xyz,i.tangent.w,i.positionDelta.xyz,i.directionDelta);}
WorldFrame packedMain(PackedInput i){
    precise float2 biased=(float2)i.weights+1.0f,weights=biased*(1.0f/32768.0f);
    return skinFrame(i.position,weights,i.indices,decodeDirection(i.frame.xy),decodeDirection(i.frame.zw),i.frame.w<128.0f?-1.0f:1.0f,i.positionDelta.xyz,i.directionDelta);
}
struct CaptureInput {
    [[vk::location(0)]] float3 position:TEXCOORD0;
    [[vk::location(1)]] float3 normal:TEXCOORD1;
    [[vk::location(2)]] float3 tangent:TEXCOORD2;
    [[vk::location(3)]] float3 binormal:TEXCOORD3;
};
struct Capture {
    float4 position:SV_Target0;float4 normal:SV_Target1;
    float4 tangent:SV_Target2;float4 binormal:SV_Target3;
};
Capture captureMain(CaptureInput i){Capture o;o.position=float4(i.position,1);o.normal=float4(i.normal,1);o.tangent=float4(i.tangent,1);o.binormal=float4(i.binormal,1);return o;}
