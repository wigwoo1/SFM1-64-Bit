// Ambient expression recovered from generic VS20 static0 dynamic2/6/10/18.
// Caller supplies a normalized world normal and explicitly selected cube.
// No local/static lights, texture multiplication, clamp or color conversion.
float3 sfm64GenericAmbientCandidate(float3 normal,float4 cube[6]){
    uint x=normal.x<0?1:0,y=normal.y<0?3:2,z=normal.z<0?5:4;
    precise float3 squared=normal*normal;
    precise float3 first=squared.y*cube[y].rgb;
    precise float3 xTerm=squared.x*cube[x].rgb;
    precise float3 second=xTerm+first;
    precise float3 zTerm=squared.z*cube[z].rgb;
    precise float3 result=zTerm+second;
    return result;
}
