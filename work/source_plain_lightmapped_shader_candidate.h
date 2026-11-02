#pragma once

// Independently authored plain no-fog LightmappedGeneric operator. This is not
// material admission or complete Source shading. No installed shader/DLL data.
// Vertex stream: POSITION float3 at0, TEXCOORD0 float2 at12, TEXCOORD1 float2
// (lightmap UV) at20, stride28. VS c4..7 are clip rows, c58..60 model rows,
// c47.w instance alpha. PS c12.rgb already includes lightmap scale; c30.x is
// separate exposure. s0 is base texture, s1 packed lightmap. Caller supplies all
// constants, textures and sampler/render state. No fog/detail/flashlight, sRGB,
// mip/format negotiation, lightstyle or automatic scene interpretation here.
// Historical fixture comments are retained to preserve tested HLSL bytes.
inline constexpr char sourcePlainLightmappedVertexShaderCandidate[]=R"SFM64(// Independent scoped vertex implementation: identity fixture, no fog/detail.
// Only outputs consumed by the documented plain pixel paths are reproduced.
float4 clipRows[4] : register(c4);
float4 modulation : register(c47);
float4 modelRows[3] : register(c58);
struct Input { float3 position:POSITION0; float2 uv:TEXCOORD0; float2 lightUV:TEXCOORD1; };
struct Output { float4 position:POSITION0; float3 uv:TEXCOORD0; float2 lightUV:TEXCOORD2; float4 world:TEXCOORD4; float4 color:COLOR0; };
Output main(Input v) {
    Output o;
    float4 p=float4(v.position,1);
    o.position=float4(dot(p,clipRows[0]),dot(p,clipRows[1]),dot(p,clipRows[2]),dot(p,clipRows[3]));
    o.world=float4(dot(p,modelRows[0]),dot(p,modelRows[1]),dot(p,modelRows[2]),o.position.z);
    o.uv=float3(v.uv,0);o.lightUV=v.lightUV;
    o.color=float4(1,1,1,modulation.w);
    return o;
}
)SFM64";
inline constexpr char sourcePlainLightmappedPixelShaderCandidate[]=R"SFM64(// Independent baseline operator. Fog and additive ambient are explicit zero.
sampler2D baseTexture:register(s0);
sampler2D lightTexture:register(s1);
float4 materialLightScale:register(c12);
float4 exposure:register(c30);
struct Input {float2 uv:TEXCOORD0;float2 lightUV:TEXCOORD2;float4 color:COLOR0;};
float4 main(Input v):COLOR0 {
    float4 base=tex2D(baseTexture,v.uv);
    float3 lighting=tex2D(lightTexture,v.lightUV).rgb*materialLightScale.rgb;
    return float4((base.rgb*lighting)*exposure.x,base.a*v.color.a);
}
)SFM64";
