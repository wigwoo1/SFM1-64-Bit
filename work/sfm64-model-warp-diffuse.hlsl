[[vk::binding(0,0)]] Texture2D<float4> baseImage;
[[vk::binding(1,0)]] SamplerState baseSampler;
[[vk::binding(8,0)]] Texture2D<float4> warpImage;
[[vk::binding(11,0)]] SamplerState warpSampler;
#ifdef NORMAL_MAP
[[vk::binding(6,0)]] Texture2D<float4> normalImage;
[[vk::binding(9,0)]] SamplerState normalSampler;
#endif
#ifdef EXPONENT_MAP
[[vk::binding(7,0)]] Texture2D<float4> exponentImage;
[[vk::binding(10,0)]] SamplerState exponentSampler;
#endif
struct Material {
    [[vk::offset(96)]] float4 fresnelBoost;
    [[vk::offset(144)]] float4 eyeExponent;
    [[vk::offset(112)]] float4 uvX;
    [[vk::offset(128)]] float4 uvY;
    // RGB stores the six ambient faces; W stores sky direction then intensity.
    [[vk::offset(160)]] float4 face[6];
};
[[vk::push_constant]] ConstantBuffer<Material> material;
float4 main([[vk::location(0)]] float2 uv:TEXCOORD0,[[vk::location(1)]] float3 normal:TEXCOORD1
#ifdef NORMAL_MAP
    , [[vk::location(3)]] float4 tangent:TEXCOORD3, [[vk::location(4)]] float3 worldPosition:TEXCOORD4
#else
    , [[vk::location(3)]] float3 worldPosition:TEXCOORD3
#endif
):SV_Target0 {
    float2 baseUV=float2(dot(float3(uv,1),material.uvX.xyz),dot(float3(uv,1),material.uvY.xyz));
    float4 base=baseImage.Sample(baseSampler,baseUV);
    float3 n=normal*rsqrt(max(dot(normal,normal),1e-16));
    float mask=0;
#ifdef NORMAL_MAP
    float3 t=tangent.xyz-n*dot(n,tangent.xyz);
    float t2=dot(t,t);
    if(abs(tangent.w)>0.5 && t2>1e-12){
        t*=rsqrt(t2);
        float4 bump=normalImage.Sample(normalSampler,uv);
        float3 m=bump.xyz*2-1;
        mask=bump.a;
        float3 mapped=t*m.x+sign(tangent.w)*cross(n,t)*m.y+n*m.z;
        if(dot(mapped,mapped)>1e-12)n=normalize(mapped);
    }
#endif
    float3 ambient=material.face[n.x<0?1:0].rgb*n.x*n.x
                  +material.face[n.y<0?3:2].rgb*n.y*n.y
                  +material.face[n.z<0?5:4].rgb*n.z*n.z;
    float3 skyDir=float3(material.face[0].w,material.face[1].w,material.face[2].w);
    float3 sky=float3(material.face[3].w,material.face[4].w,material.face[5].w);
    uint w,h;warpImage.GetDimensions(w,h);
    float warpCoordinate=saturate(0.5+0.5*dot(n,skyDir));
    float2 edge=0.5/float2(w,h);
    float3 warp=warpImage.SampleLevel(warpSampler,clamp(warpCoordinate.xx,edge,1-edge),0).rgb;
    float3 rgb=base.rgb*(ambient+2*sky*warp);
    if(material.fresnelBoost.w<0)mask=base.a;
    float exponent=material.eyeExponent.w;
#ifdef EXPONENT_MAP
    if(exponent==0)exponent=1+149*exponentImage.Sample(exponentSampler,uv).r;
#endif
    if(exponent>0 && (mask>0 || material.uvX.w>0)){
        float3 eye=material.eyeExponent.xyz-worldPosition;
        float3 view=eye*rsqrt(max(dot(eye,eye),1e-16));
        float f=1-saturate(dot(n,view));f*=f;
        float3 ranges=material.fresnelBoost.xyz;
        float fresnel=f<0.5?lerp(ranges.x,ranges.y,2*f):lerp(ranges.y,ranges.z,2*f-1);
        float reflected=saturate(dot(reflect(-view,n),skyDir));
        float3 direct=sky*saturate(dot(n,skyDir));
        float3 highlight=direct*(mask*abs(material.fresnelBoost.w)*fresnel*pow(reflected,exponent));
        if(material.uvX.w>0){
            float rimWeight=f*f;
            float3 directRim=direct*(rimWeight*pow(reflected,material.uvX.w));
            float3 ambientView=material.face[view.x<0?1:0].rgb*view.x*view.x
                              +material.face[view.y<0?3:2].rgb*view.y*view.y
                              +material.face[view.z<0?5:4].rgb*view.z*view.z;
            highlight=max(highlight,directRim)+ambientView*(rimWeight*material.uvY.w*saturate(n.z));
        }
        rgb+=highlight;
    }
    return float4(rgb,base.a);
}
