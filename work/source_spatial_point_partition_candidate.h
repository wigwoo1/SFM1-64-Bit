#pragma once
#include "source_displacement_bounds_gate_candidate.h"
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>
struct SourceSpatialPointEntryCandidate {
    uint64_t entityId=0;
    uint32_t mask=0;
    std::array<float,6>bounds{}; // Unexpanded physics minsXYZ/maxsXYZ.
};
// Immutable point-ray partition. Original 4levels, mask split, cell list order,
// quantization and DDA; owned integer indices/64bit scene IDs, no native pointers.
// Dynamic move/remove, hull queries and mutation from callbacks are separate.
class SourceSpatialPointPartitionCandidate {
    struct Record {uint64_t id=0;uint32_t mask=0,flags=0;std::array<float,6>box{};};
    using Cells=std::unordered_map<uint32_t,std::vector<uint32_t>>;
    std::array<std::array<Cells,4>,2>trees_;
    std::vector<Record>records_;
    static float clamp(float x){return x>16384.f?16384.f:x< -16384.f?-16384.f:x;}
    static uint32_t key(int x,int y,int z){return (uint32_t(x)&2047u)|((uint32_t(y)&2047u)<<11)|(uint32_t(z)<<22);}
    static uint32_t parent(uint32_t k){return (k>>2)&0x3fcff9ffu;}
    static int coordinate(float x,unsigned level){const float relative=x-(-16384.f);if(!std::isfinite(relative)||relative< -2147483648.f||relative>=2147483648.f)throw std::runtime_error("spatial coordinate exceeds checked CVTTSS2SI domain");return int(relative)>>int(8+level*2);}
    static void finite(float x){if(!std::isfinite(x))throw std::runtime_error("nonfinite spatial partition arithmetic");}
    static bool worldClip(std::array<float,3>&start,std::array<float,3>&delta){
        std::array<float,3>end{};for(unsigned j=0;j<3;++j){end[j]=start[j]+delta[j];finite(end[j]);}
        auto inside=[](const auto&p){for(float x:p)if(x< -16384.f||x>16384.f)return false;return true;};const bool startIn=inside(start),endIn=inside(end);
        if(!startIn&&!endIn)return false;
        // Preserve original1BDDB0/1BDF90 operation grouping/asymmetry, including
        // 5-unit inward start clipping and both-endpoints-outside refusal.
        if(!startIn){
            for(unsigned j=0;j<3;++j){if(std::abs(double(delta[j]))<1e-10)continue;float t=0;bool clip=false;
                if(delta[j]>0.f&&start[j]< -16384.f){t=(-16379.f-start[j])/delta[j];clip=true;}
                else if(delta[j]<=0.f&&start[j]>16384.f){t=(start[j]-16379.f)*(-1.f/delta[j]);clip=true;}
                if(clip)for(unsigned k=0;k<3;++k){start[k]=delta[k]*t+start[k];finite(start[k]);}
            }for(unsigned j=0;j<3;++j)delta[j]=end[j]-start[j];
        }else if(!endIn){
            for(unsigned j=0;j<3;++j){if(std::abs(double(delta[j]))<1e-10)continue;float t=0;bool clip=false;
                if(delta[j]<0.f&&end[j]< -16384.f){t=(start[j]-(-16379.f))*(-1.f/delta[j]);clip=true;}
                else if(delta[j]>=0.f&&end[j]>16384.f){t=(start[j]-(-16379.f))/delta[j];clip=true;}
                if(clip)for(unsigned k=0;k<3;++k){end[k]=delta[k]*t+start[k];finite(end[k]);}
            }for(unsigned j=0;j<3;++j)delta[j]=end[j]-start[j];
        }
        for(float x:delta)finite(x);return true;
    }
public:
    explicit SourceSpatialPointPartitionCandidate(const std::vector<SourceSpatialPointEntryCandidate>&entries){
        if(entries.size()>65535)throw std::runtime_error("spatial handle domain exhausted");records_.reserve(entries.size());std::unordered_set<uint64_t>ids;
        for(const auto&e:entries){
            if(!e.entityId||!ids.insert(e.entityId).second||e.mask>65535)throw std::runtime_error("spatial identity/mask invalid");for(float x:e.bounds)finite(x);for(unsigned j=0;j<3;++j)if(e.bounds[j]>e.bounds[j+3])throw std::runtime_error("spatial input box inverted");
            Record r;r.id=e.entityId;r.mask=e.mask;r.flags=(e.mask&0x1ac?2u:0u)|(e.mask&0xfe53?4u:0u);
            // Original CreateHandle uses FLT_MIN (small positive), not -FLT_MAX,
            // for the uninserted mask0 maximum fields.
            r.box={std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::min(),std::numeric_limits<float>::min(),std::numeric_limits<float>::min()};
            if(e.mask)for(unsigned j=0;j<6;++j)r.box[j]=clamp(e.bounds[j]+(j<3?-.03125f:.03125f));
            const auto index=uint32_t(records_.size());records_.push_back(r);if(!e.mask)continue;
            unsigned level=0;for(;level<3;++level){const float size=float(256u<<(level*2));if(size>r.box[3]-r.box[0]&&size>r.box[4]-r.box[1]&&size>r.box[5]-r.box[2])break;}
            for(unsigned tree=0;tree<2;++tree)if(r.flags&(2u<<tree)){
                auto&cells=trees_[tree][level];for(int x=coordinate(r.box[0],level);x<=coordinate(r.box[3],level);++x)for(int y=coordinate(r.box[1],level);y<=coordinate(r.box[4],level);++y)for(int z=coordinate(r.box[2],level);z<=coordinate(r.box[5],level);++z)cells[key(x,y,z)].push_back(index);
            }
        }
    }
    size_t size()const{return records_.size();}
    const auto&storedBounds(size_t i)const{return records_.at(i).box;}
    uint32_t storedMask(size_t i)const{return records_.at(i).mask;}
    uint32_t storedFlags(size_t i)const{return records_.at(i).flags;}
    std::vector<uint64_t>queryPoint(const std::array<float,3>&inputStart,const std::array<float,3>&inputDelta,uint32_t mask,size_t stopAfter=0)const{
        if(mask>65535)throw std::runtime_error("spatial query mask outside supported domain");for(const auto&v:{inputStart,inputDelta})for(float x:v)finite(x);if(!mask)return {};
        const float squared=(inputDelta[0]*inputDelta[0]+inputDelta[1]*inputDelta[1])+inputDelta[2]*inputDelta[2];finite(squared);
        return queryPointRay(inputStart,inputDelta,squared!=0.f,mask,stopAfter);
    }
    // TraceRay copies m_IsSwept even when world clipping shortens delta to0.
    // Scene adapters must pass that flag, not derive it from the clipped delta.
    std::vector<uint64_t>queryPointRay(const std::array<float,3>&inputStart,const std::array<float,3>&inputDelta,bool swept,uint32_t mask,size_t stopAfter=0)const{
        if(mask>65535)throw std::runtime_error("spatial query mask outside supported domain");for(const auto&v:{inputStart,inputDelta})for(float x:v)finite(x);
        std::vector<uint64_t>answer;if(!mask)return answer;std::vector<bool>visited(records_.size(),false);const unsigned tree=mask&0x1ac?0:1;
        auto start=inputStart,delta=inputDelta;
        if(swept&&!worldClip(start,delta))return answer;
        std::array<float,3>inverse{};if(swept)for(unsigned j=0;j<3;++j){inverse[j]=delta[j]!=0.f?1.f/delta[j]:std::numeric_limits<float>::max();finite(inverse[j]);}
        auto visit=[&](unsigned level,uint32_t cell){
            const auto found=trees_[tree][level].find(cell);if(found==trees_[tree][level].end())return true;
            for(auto it=found->second.rbegin();it!=found->second.rend();++it){const auto i=*it;const auto&r=records_[i];if(!(r.mask&mask)||visited[i])continue;visited[i]=true;bool hit=true;
                if(swept){const std::array<float,3>lo{r.box[0],r.box[1],r.box[2]},hi{r.box[3],r.box[4],r.box[5]};hit=sourceDisplacementBoundsGateCandidate(lo,hi,start,delta,inverse,0.f);}
                // Original stationary ray delegates to EnumerateInBox, which
                // clamps the query point before both lookup AND overlap tests.
                else for(unsigned j=0;j<3;++j)if(clamp(inputStart[j])<r.box[j]||clamp(inputStart[j])>r.box[j+3])hit=false;
                if(hit){answer.push_back(r.id);if(stopAfter&&answer.size()>=stopAfter)return false;}
            }return true;
        };
        if(!swept){
            for(unsigned level=0;level<4;++level){const uint32_t cell=key(coordinate(clamp(start[0]),level),coordinate(clamp(start[1]),level),coordinate(clamp(start[2]),level));if(!visit(level,cell))break;}return answer;
        }
        std::array<float,3>end{};for(unsigned j=0;j<3;++j){end[j]=start[j]+delta[j];finite(end[j]);}
        uint32_t cell=key(coordinate(start[0],0),coordinate(start[1],0),coordinate(start[2],0));const uint32_t endCell=key(coordinate(end[0],0),coordinate(end[1],0),coordinate(end[2],0));
        std::array<int,3>step{};std::array<float,3>next{},increment{};
        for(unsigned j=0;j<3;++j){const float s=start[j]-(-16384.f),e=end[j]-(-16384.f);if(s==e){next[j]=increment[j]=65536.f;continue;}step[j]=delta[j]<0.f?-1:1;const int c=int(s)>>8;const float boundary=float(int32_t(uint32_t(c+(step[j]>0?1:0))*256u));const float a=step[j]<0?s-boundary:boundary-s,b=step[j]<0?e-boundary:boundary-e;
            if(a>0&&b>0)next[j]=increment[j]=65536.f;else{const float inv=step[j]<0?-inverse[j]:inverse[j];next[j]=a*inv;increment[j]=256.f*inv;}finite(next[j]);finite(increment[j]);
        }
        std::array<uint32_t,4>last{UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX};
        for(size_t iteration=0;;++iteration){uint32_t at=cell;
            for(unsigned level=0;level<4;++level){if(level==0||last[level]!=at){last[level]=at;if(!visit(level,at))return answer;}at=parent(at);}
            if(cell==endCell&&iteration==0)return answer;
            if(next[0]>=1.f&&next[1]>=1.f&&next[2]>=1.f)return answer;
            const unsigned axis=next[1]>next[0]?(next[2]>next[0]?0:2):(next[2]>next[1]?1:2);const float previous=next[axis];next[axis]=increment[axis]+next[axis];if(!(next[axis]>previous))throw std::runtime_error("spatial DDA arithmetic made no progress");
            const unsigned shift=axis*11;if(axis<2){const uint32_t changed=((cell+(uint32_t(step[axis])<<shift))^cell)&(2047u<<shift);cell^=changed;}else cell+=uint32_t(step[axis])<<22;
        }
    }
};
