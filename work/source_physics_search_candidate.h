#pragma once
#include "source_physics_tetrahedron_simplex_candidate.h"
#include "source_physics_contact_plane_candidate.h"

// Pointer-free semantic trace. surfaceId0 denotes original "**empty**";
// entityId is an identifier, never an x86 address to dereference. Native32
// layout conversion lives exclusively in the test adapter.
struct SourcePhysicsSearchTraceCandidate {
    std::array<float,3>start{},end{};
    SourcePhysicsContactPlaneCandidate plane;
    std::array<uint8_t,4>planeTags{};
    float fraction=1;
    uint32_t contents=0;
    uint16_t displacementFlags=0;
    uint8_t allSolid=0,startSolid=0;
    float fractionLeftSolid=0;
    uint64_t surfaceId=0;
    uint32_t surfaceMetadata=0,hitGroup=0;
    uint16_t physicsBone=0,worldSurface=0;
    uint64_t entityId=0;
    uint32_t hitBox=0;
};
struct SourcePhysicsSearchStateCandidate {
    SourcePhysicsSearchTraceCandidate trace;
    std::array<float,3>direction{};
    float distance=0;
};
struct SourcePhysicsSearchStatisticsCandidate {
    uint32_t outerIterations=0,innerIterations=0,refinementCalls=0,planeCalls=0,timingCalls=0,exitCode=0;
};
// Complete original296B0 finite-result policy. False returns commit observed
// direction/cache changes but preserve the old trace and distance. Exceptions
// preserve all caller state. This is a convex search, not tree traversal.
inline bool sourcePhysicsSearchCandidate(SourcePhysicsSearchStateCandidate&state,
    const std::array<float,16>&ray,const SourcePhysicsBoxSupportCandidate&box,
    const SourcePhysicsSupportContextCandidate&shape,float tolerance,
    SourcePhysicsSupportCacheCandidate&cache,SourcePhysicsSearchStatisticsCandidate*statistics=nullptr){
    using V=std::array<float,3>;
    auto finite=[](float x){if(!std::isfinite(x))throw std::runtime_error("nonfinite physics search arithmetic");return x;};
    finite(tolerance);for(float x:state.direction)finite(x);
    auto result=state;auto nextCache=cache;SourcePhysicsSearchStatisticsCandidate stats;
    auto finish=[&](bool hit,uint32_t code){stats.exitCode=code;state=result;cache=nextCache;if(statistics)*statistics=stats;return hit;};
    auto dot=[&](const V&a,const V&b){for(float x:a)finite(x);for(float x:b)finite(x);return finite((a[1]*b[1]+a[0]*b[0])+a[2]*b[2]);};
    auto searchDirection=[&](){return sourcePhysicsContactNormalizeCandidate(V{-result.direction[0],-result.direction[1],-result.direction[2]});};
    auto sample=[&](const V&direction){
        const auto a=sourcePhysicsBoxSupportCandidate(box,direction);const bool end=dot(V{ray[6],ray[7],ray[8]},direction)>0;const V opposite{-direction[0],-direction[1],-direction[2]};const auto b=sourcePhysicsSupportCandidate(shape,opposite,nextCache);
        SourcePhysicsSimplexVertexCandidate incoming;for(unsigned i=0;i<3;++i)incoming.point[i]=finite(finite(a.point[i]+finite(ray[(end?3:0)+i]))-b.point[i]);incoming.featureA=uint16_t((a.feature&0x7fff)|(end?0x8000:0));incoming.featureB=uint16_t(b.feature);return incoming;
    };
    if(result.direction[0]==0&&result.direction[1]==0&&result.direction[2]==0)result.direction={1,0,0};
    V direction=searchDirection();SourcePhysicsSimplexCandidate simplex;
    auto reduce=[&](const SourcePhysicsSimplexVertexCandidate&incoming){switch(simplex.count){
        case 0:simplex.count=1;simplex.vertices[0]=incoming;result.direction=incoming.point;return false;
        case 1:return sourcePhysicsLineSimplexCandidate(simplex,incoming,result.direction);
        case 2:return sourcePhysicsTriangleSimplexCandidate(simplex,incoming,result.direction);
        case 3:return sourcePhysicsTetrahedronSimplexCandidate(simplex,incoming,result.direction);
        default:return true;
    }};
    for(unsigned outer=0;outer<100;++outer){
        ++stats.outerIterations;auto incoming=sample(direction);const float projection=dot(incoming.point,direction);
        if(projection<0){
            float best=std::fabs(projection);if(!(tolerance>best))return finish(false,1);if(!(finite(ray[12])>0))return finish(false,2);V bestDirection=direction;
            if(best>0)for(unsigned inner=0;inner<20;++inner){
                ++stats.innerIterations;const V previous=incoming.point;reduce(incoming);direction=searchDirection();incoming=sample(direction);const float candidate=-dot(incoming.point,direction);
                if(candidate>tolerance)return finish(false,3);if(candidate>best){best=candidate;bestDirection=direction;}
                const float old=-dot(previous,direction);if(finite(candidate-old)>-9.999999747378752e-5f)break;
            }
            // Original keeps products and sums in default53bit x87 precision,
            // with a separate rounded float used by the SSE tests/division.
            for(unsigned i=0;i<3;++i)finite(ray[6+i]);const double motion=-((double(ray[7])*bestDirection[1]+double(ray[6])*bestDirection[0])+double(ray[8])*bestDirection[2]);const float roundedMotion=finite(float(motion));
            if(!((-double(tolerance)*.1)>motion)&&!((-9.999999747378752e-5f>roundedMotion)&&(double(tolerance)*.9>double(best))))return finish(false,4);
            result.trace={};const float inverse=finite(-1.f/roundedMotion);const float step=finite(finite(finite(tolerance-best)*finite(ray[13]))*inverse);result.distance=finite(finite(ray[12])-step);
            if(result.distance<0){result.distance=0;float backoff=0;if(simplex.count==3){++stats.timingCalls;backoff=sourcePhysicsTriangleContactTimeCandidate(simplex,V{ray[9],ray[10],ray[11]},tolerance);}if(ray[12]>backoff)result.distance=finite(ray[12]-backoff);}
            result.trace.plane.normal={-bestDirection[0],-bestDirection[1],-bestDirection[2]};result.trace.contents=1;return finish(true,5);
        }
        if(simplex.count>3||reduce(incoming)){
            result.trace={};
            if(finite(ray[12])!=0){
                ++stats.refinementCalls;const float time=sourcePhysicsRefinementCandidate(simplex,ray,shape,box,nextCache);
                if(ray[12]>time&&time>0){result.distance=finite(ray[12]-time);++stats.planeCalls;result.trace.plane=sourcePhysicsContactPlaneCandidate(simplex,ray,box,shape,result.trace.end);const float facing=dot(V{ray[9],ray[10],ray[11]},result.trace.plane.normal);if(facing<0)result.distance=finite(result.distance+finite(tolerance/facing));if(result.distance<0)result.distance=0;result.trace.contents=1;return finish(true,6);}
            }
            result.distance=0;result.trace.allSolid=1;result.trace.startSolid=1;return finish(true,7);
        }
        direction=searchDirection();
    }
    return finish(false,8);
}
