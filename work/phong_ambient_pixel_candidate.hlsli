// Independently owned expression for one explicit Phong ambient permutation.
// Inputs are sampled linear RGBA, interpolated world T/B/N/position and constants.
// Not a complete Phong material, texture system, or active SFM shader selector.
struct PhongAmbientPixelRecord {
    float4 base,normalSample,tangent,binormal,normal,position;
    float4 diffuse,eye,fog,controls,fogColor,lightScale;
    float4 cube[6];
};
float4 phongAmbientPixelCandidate(PhongAmbientPixelRecord r){
    precise float3 decoded=2.0f*r.normalSample.xyz-1.0f;
    precise float3 by=decoded.y*r.binormal.xyz;
    precise float3 xy=r.tangent.xyz*decoded.x+by;
    precise float3 transformed=r.normal.xyz*decoded.z+xy;
    float3 n=normalize(transformed);
    float3 negative=float3(n.x<0?1.0f:0.0f,n.y<0?1.0f:0.0f,n.z<0?1.0f:0.0f);
    precise float3 squared=n*n;
    precise float3 positive=1.0f-negative;
    precise float3 nw=negative*squared,pw=squared*positive;
    // Preserve the recovered -X,+X,+Y,-Y,+Z,-Z sum order.
    precise float3 ambient=nw.x*r.cube[1].xyz;
    ambient=pw.x*r.cube[0].xyz+ambient;
    ambient=pw.y*r.cube[2].xyz+ambient;
    ambient=nw.y*r.cube[3].xyz+ambient;
    ambient=pw.z*r.cube[4].xyz+ambient;
    ambient=nw.z*r.cube[5].xyz+ambient;
    float modulationWeight=saturate(r.base.w+r.controls.z);
    precise float3 modulation=modulationWeight*(r.diffuse.xyz-1.0f)+1.0f;
    precise float3 lit=ambient*modulation;
    precise float3 color=r.base.xyz*lit;
    float distance=length(r.eye.xyz-r.position.xyz);
    float fogWeight=min(saturate(distance*r.fog.w+r.fog.x),r.fog.z);
    precise float fogSquared=fogWeight*fogWeight;
    precise float3 difference=color*(-r.lightScale.x)+r.fogColor.xyz;
    precise float3 fogged=fogSquared*difference;
    precise float3 rgb=color*r.lightScale.x+fogged;
    precise float alphaWeight=(1.0f-r.controls.x)*r.controls.z;
    precise float alpha=(alphaWeight*(r.base.w-1.0f)+1.0f)*r.diffuse.w;
    return float4(rgb,alpha);
}
